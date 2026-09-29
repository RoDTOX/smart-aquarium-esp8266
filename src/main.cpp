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
            --bg-base: #0284c7;
            --bg-gradient: linear-gradient(180deg, #7dd3fc 0%, #38bdf8 12%, #0ea5e9 32%, #0284c7 62%, #0369a1 88%, #075985 100%);
            --bg-surface: rgba(255, 255, 255, 0.72);
            --bg-card: rgba(255, 255, 255, 0.78);
            --text-primary: #0f172a;
            --text-secondary: #334155;
            --text-muted: #64748b;
            --accent-primary: #0284c7;
            --accent-primary-glow: rgba(2, 132, 199, 0.35);
            --ocean-blue: #0284c7;
            --ocean-deep: #0369a1;
            --state-ok: #10b981;
            --state-ok-glow: rgba(16, 185, 129, 0.35);
            --state-warn: #f59e0b;
            --state-warn-glow: rgba(245, 158, 11, 0.35);
            --state-danger: #ef4444;
            --state-danger-glow: rgba(239, 68, 68, 0.35);
            --border-color: rgba(255, 255, 255, 0.85);
            --border-subtle: rgba(2, 132, 199, 0.18);
            --track-bg: rgba(0, 0, 0, 0.04);
            --slot-off-bg: rgba(255, 255, 255, 0.55);
            --slot-off-border: rgba(148, 163, 184, 0.4);
            --label-bg: rgba(255, 255, 255, 0.96);
            --modal-bg: rgba(255, 255, 255, 0.95);
            --input-bg: rgba(255, 255, 255, 0.9);
            --btn-bg: rgba(255, 255, 255, 0.85);
        }

        [data-theme="dark"] {
            --bg-base: #060e18;
            --bg-gradient: linear-gradient(180deg, #091728 0%, #06111f 25%, #040c17 60%, #02070e 100%);
            --bg-surface: rgba(11, 26, 44, 0.82);
            --bg-card: rgba(13, 31, 53, 0.86);
            --text-primary: #f8fafc;
            --text-secondary: #cbd5e1;
            --text-muted: #94a3b8;
            --accent-primary: #38bdf8;
            --accent-primary-glow: rgba(56, 189, 248, 0.4);
            --ocean-blue: #38bdf8;
            --ocean-deep: #7dd3fc;
            --state-ok: #10b981;
            --state-ok-glow: rgba(16, 185, 129, 0.5);
            --state-warn: #fbbf24;
            --state-warn-glow: rgba(251, 191, 36, 0.5);
            --state-danger: #f87171;
            --state-danger-glow: rgba(248, 113, 113, 0.5);
            --border-color: rgba(56, 189, 248, 0.26);
            --border-subtle: rgba(56, 189, 248, 0.14);
            --track-bg: rgba(0, 0, 0, 0.38);
            --slot-off-bg: rgba(15, 23, 42, 0.65);
            --slot-off-border: rgba(51, 65, 85, 0.65);
            --label-bg: rgba(13, 31, 53, 0.95);
            --modal-bg: rgba(11, 26, 44, 0.96);
            --input-bg: rgba(15, 23, 42, 0.85);
            --btn-bg: rgba(15, 23, 42, 0.75);
        }
        
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body {
            font-family: 'Outfit', -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background: var(--bg-gradient);
            background-attachment: fixed;
            color: var(--text-primary);
            padding: 16px 12px 90px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
            position: relative;
            overflow-x: hidden;
            transition: background 0.4s ease, color 0.3s ease;
        }
        
        /* Sunbeams / God rays radiating from water surface */
        body::before {
            content: "";
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: repeating-conic-gradient(from 180deg at 50% -70px, rgba(255, 255, 255, 0.18) 0deg, rgba(255, 255, 255, 0.02) 11deg, rgba(255, 255, 255, 0.18) 22deg);
            mask-image: linear-gradient(180deg, rgba(0,0,0,0.85) 0%, rgba(0,0,0,0.3) 45%, transparent 75%);
            -webkit-mask-image: linear-gradient(180deg, rgba(0,0,0,0.85) 0%, rgba(0,0,0,0.3) 45%, transparent 75%);
            pointer-events: none;
            z-index: 0;
            opacity: 0.8;
        }
        [data-theme="dark"] body::before {
            background: repeating-conic-gradient(from 180deg at 50% -70px, rgba(56, 189, 248, 0.12) 0deg, rgba(2, 132, 199, 0.01) 11deg, rgba(56, 189, 248, 0.12) 22deg);
            opacity: 0.55;
        }
        
        /* Floating animated bubbles */
        .bubbles-layer {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            pointer-events: none;
            overflow: hidden;
            z-index: 1;
        }
        .bubble {
            position: absolute;
            bottom: -50px;
            border-radius: 50%;
            background: radial-gradient(circle at 34% 32%, rgba(255,255,255,0.95) 0%, rgba(255,255,255,0.32) 48%, rgba(255,255,255,0.05) 72%, rgba(255,255,255,0.65) 100%);
            box-shadow: inset 0 0 5px rgba(255,255,255,0.75), 0 0 6px rgba(56, 189, 248, 0.3);
            pointer-events: none;
            will-change: transform, opacity;
        }
        [data-theme="dark"] .bubble {
            background: radial-gradient(circle at 34% 32%, rgba(255,255,255,0.96) 0%, rgba(224,242,254,0.38) 45%, rgba(56,189,248,0.08) 72%, rgba(56,189,248,0.7) 100%);
            box-shadow: inset 0 0 6px rgba(255,255,255,0.85), 0 0 9px rgba(56, 189, 248, 0.45);
        }
        .bubble-wobble-1 { animation: riseWobble1 linear infinite; }
        .bubble-wobble-2 { animation: riseWobble2 linear infinite; }
        .bubble-fast { animation: riseFastMicro linear infinite; }

        @keyframes riseWobble1 {
            0%   { transform: translateY(0) translateX(0) scale(0.65); opacity: 0; }
            8%   { opacity: 0.9; }
            30%  { transform: translateY(-36vh) translateX(-12px) scale(0.85); }
            65%  { transform: translateY(-72vh) translateX(10px) scale(1.02); }
            88%  { opacity: 0.85; }
            100% { transform: translateY(-118vh) translateX(-6px) scale(1.15); opacity: 0; }
        }
        @keyframes riseWobble2 {
            0%   { transform: translateY(0) translateX(0) scale(0.65); opacity: 0; }
            8%   { opacity: 0.9; }
            30%  { transform: translateY(-36vh) translateX(14px) scale(0.85); }
            65%  { transform: translateY(-72vh) translateX(-10px) scale(1.02); }
            88%  { opacity: 0.85; }
            100% { transform: translateY(-118vh) translateX(8px) scale(1.15); opacity: 0; }
        }
        @keyframes riseFastMicro {
            0%   { transform: translateY(0) translateX(0) scale(0.6); opacity: 0; }
            10%  { opacity: 0.85; }
            45%  { transform: translateY(-52vh) translateX(7px) scale(0.85); }
            85%  { opacity: 0.85; }
            100% { transform: translateY(-118vh) translateX(-4px) scale(1.0); opacity: 0; }
        }

        /* Natural Volcanic Aquasoil Substrate Bed (from user's real tank photo) */
        .aquasoil-bed {
            position: fixed;
            bottom: 0;
            left: 0;
            width: 100%;
            height: 52px;
            pointer-events: none;
            z-index: 1;
            background: linear-gradient(180deg, transparent 0%, rgba(45, 27, 20, 0.35) 25%, rgba(24, 15, 11, 0.75) 60%, #150d09 100%);
            box-shadow: inset 0 3px 8px rgba(0, 0, 0, 0.4);
        }
        [data-theme="dark"] .aquasoil-bed {
            background: linear-gradient(180deg, transparent 0%, rgba(15, 9, 6, 0.5) 25%, rgba(10, 6, 4, 0.85) 60%, #050302 100%);
        }

        /* Floating Plant Roots Fringe at Surface (Salvinia / Frogbit) */
        .floating-roots-layer {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 48px;
            pointer-events: none;
            z-index: 1;
            overflow: hidden;
            opacity: 0.85;
        }
        .roots-svg {
            width: 100%;
            height: 100%;
            display: block;
        }

        /* Swaying Aquatic Plants (Staurogyne / Ludwigia / Vallisneria) */
        .plants-layer {
            position: fixed;
            bottom: 0;
            left: 0;
            width: 100%;
            height: 280px;
            pointer-events: none;
            z-index: 1;
            overflow: hidden;
        }
        .plant {
            position: absolute;
            bottom: 0;
            height: 260px;
            width: 120px;
            transform-origin: bottom center;
            opacity: 0.65;
            transition: opacity 0.3s;
        }
        .plant-left {
            left: 0;
            animation: swayPlantLeft 7s ease-in-out infinite;
        }
        .plant-right {
            right: 0;
            animation: swayPlantRight 8.5s ease-in-out infinite 1s;
        }
        [data-theme="dark"] .plant {
            opacity: 0.42;
            filter: brightness(0.85) drop-shadow(0 0 8px rgba(34, 197, 94, 0.2));
        }
        @keyframes swayPlantLeft {
            0%, 100% { transform: rotate(0deg) skewX(0deg); }
            50% { transform: rotate(3deg) skewX(2.5deg); }
        }
        @keyframes swayPlantRight {
            0%, 100% { transform: rotate(0deg) skewX(0deg); }
            50% { transform: rotate(-3.5deg) skewX(-3deg); }
        }

        /* Realistic Inhabitants (Strictly swimming head-first: scaleX(-1)=heading Right, scaleX(1)=heading Left) */
        .critter {
            position: fixed;
            pointer-events: none;
            z-index: 1;
            filter: drop-shadow(0 3px 5px rgba(0,0,0,0.18));
            opacity: 0.9;
            will-change: transform, left;
        }
        [data-theme="dark"] .critter {
            filter: drop-shadow(0 0 6px rgba(56, 189, 248, 0.35));
        }

        /* Guppy tail wave motion */
        .guppy-tail {
            transform-origin: 36px 14px;
            animation: guppyTailWave 0.8s ease-in-out infinite alternate;
        }
        @keyframes guppyTailWave {
            0% { transform: scaleY(1.0) rotate(0deg); }
            100% { transform: scaleY(0.82) rotate(-8deg); }
        }

        /* Critter Positions & Trajectories */
        .critter-mickey {
            top: 65%;
            animation: swimCritterMickey 26s ease-in-out infinite;
        }
        .critter-guppy {
            top: 26%;
            animation: swimCritterGuppy 32s ease-in-out infinite 2s;
        }
        .critter-fry {
            top: 15%;
            animation: swimCritterFry 22s ease-in-out infinite 0.5s;
        }
        .critter-sae-1 {
            top: 42%;
            animation: swimCritterSAE1 28s ease-in-out infinite 4s;
        }
        .critter-sae-2 {
            top: 78%;
            animation: swimCritterSAE2 34s ease-in-out infinite 9s;
        }
        .critter-shrimp {
            bottom: 40px;
            animation: roamShrimp 38s ease-in-out infinite;
        }
        .critter-snail {
            bottom: 12px;
            left: 24%;
            opacity: 0.82;
            animation: crawlSnail 60s linear infinite alternate;
        }

        /* Swimming Trajectories: Heading Right = scaleX(-1), Heading Left = scaleX(1) */
        @keyframes swimCritterMickey {
            0%   { left: 3%; transform: scaleX(-1) translateY(0); }
            47%  { left: calc(100% - 60px); transform: scaleX(-1) translateY(-12px); }
            50%  { left: calc(100% - 60px); transform: scaleX(1) translateY(-12px); }
            97%  { left: 3%; transform: scaleX(1) translateY(10px); }
            100% { left: 3%; transform: scaleX(-1) translateY(0); }
        }
        @keyframes swimCritterGuppy {
            0%   { left: calc(100% - 65px); transform: scaleX(1) translateY(0); }
            47%  { left: 4%; transform: scaleX(1) translateY(14px); }
            50%  { left: 4%; transform: scaleX(-1) translateY(14px); }
            97%  { left: calc(100% - 65px); transform: scaleX(-1) translateY(-10px); }
            100% { left: calc(100% - 65px); transform: scaleX(1) translateY(0); }
        }
        @keyframes swimCritterFry {
            0%   { left: 6%; transform: scaleX(-1) translateY(0); }
            46%  { left: calc(100% - 50px); transform: scaleX(-1) translateY(-8px); }
            50%  { left: calc(100% - 50px); transform: scaleX(1) translateY(-8px); }
            96%  { left: 6%; transform: scaleX(1) translateY(6px); }
            100% { left: 6%; transform: scaleX(-1) translateY(0); }
        }
        @keyframes swimCritterSAE1 {
            0%   { left: 2%; transform: scaleX(-1) translateY(0); }
            42%  { left: 75%; transform: scaleX(-1) translateY(-14px); }
            46%  { left: 75%; transform: scaleX(1) translateY(-14px); }
            70%  { left: 32%; transform: scaleX(1) translateY(8px); }
            74%  { left: 32%; transform: scaleX(1) translateY(8px); }
            96%  { left: 2%; transform: scaleX(1) translateY(0); }
            100% { left: 2%; transform: scaleX(-1) translateY(0); }
        }
        @keyframes swimCritterSAE2 {
            0%   { left: calc(100% - 65px); transform: scaleX(1) translateY(0); }
            47%  { left: 5%; transform: scaleX(1) translateY(8px); }
            50%  { left: 5%; transform: scaleX(-1) translateY(8px); }
            97%  { left: calc(100% - 65px); transform: scaleX(-1) translateY(-8px); }
            100% { left: calc(100% - 65px); transform: scaleX(1) translateY(0); }
        }
        @keyframes roamShrimp {
            0%   { left: 14%; transform: scaleX(-1) translateY(0); }
            46%  { left: 72%; transform: scaleX(-1) translateY(-10px); }
            50%  { left: 72%; transform: scaleX(1) translateY(-10px); }
            96%  { left: 14%; transform: scaleX(1) translateY(0); }
            100% { left: 14%; transform: scaleX(-1) translateY(0); }
        }
        @keyframes crawlSnail {
            0%   { left: 24%; transform: translateY(0); }
            100% { left: 32%; transform: translateY(-2px); }
        }
        
        .container {
            max-width: 900px;
            width: 100%;
            display: flex;
            flex-direction: column;
            gap: 18px;
            position: relative;
            z-index: 2;
        }
        
        header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            background: rgba(255, 255, 255, 0.75);
            backdrop-filter: blur(18px);
            -webkit-backdrop-filter: blur(18px);
            padding: 14px 20px;
            border-radius: 16px;
            border: 1.5px solid var(--border-color);
            box-shadow: 0 8px 24px rgba(2, 132, 199, 0.15);
        }
        [data-theme="dark"] header {
            background: rgba(10, 24, 43, 0.88);
            border-color: rgba(56, 189, 248, 0.28);
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5), 0 0 20px rgba(56, 189, 248, 0.08);
        }
        
        h1 {
            font-size: 22px;
            font-weight: 800;
            background: linear-gradient(135deg, #0284c7, #0369a1);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .sys-badge {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 13px;
            font-weight: 700;
            padding: 6px 12px;
            border-radius: 9999px;
            background: rgba(255, 255, 255, 0.85);
            border: 1.5px solid rgba(16, 185, 129, 0.4);
            color: #065f46;
            box-shadow: 0 2px 6px rgba(16, 185, 129, 0.15);
        }
        [data-theme="dark"] .sys-badge {
            background: rgba(6, 78, 59, 0.4);
            border-color: rgba(16, 185, 129, 0.5);
            color: #34d399;
            box-shadow: 0 0 12px rgba(16, 185, 129, 0.25);
        }
        
        .status-dot {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            display: inline-block;
            flex-shrink: 0;
        }
        
        @keyframes pulse-green {
            0% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.5); }
            70% { box-shadow: 0 0 0 6px rgba(16, 185, 129, 0); }
            100% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
        }
        @keyframes pulse-orange {
            0% { box-shadow: 0 0 0 0 rgba(245, 158, 11, 0.5); }
            70% { box-shadow: 0 0 0 6px rgba(245, 158, 11, 0); }
            100% { box-shadow: 0 0 0 0 rgba(245, 158, 11, 0); }
        }
        @keyframes pulse-red {
            0% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0.5); }
            70% { box-shadow: 0 0 0 6px rgba(239, 68, 68, 0); }
            100% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0); }
        }
        
        .dot-green { background-color: var(--state-ok); animation: pulse-green 2s infinite; }
        .dot-orange { background-color: var(--state-warn); animation: pulse-orange 2s infinite; }
        .dot-red { background-color: var(--state-danger); animation: pulse-red 2s infinite; }
        
        .card {
            background: var(--bg-card);
            backdrop-filter: blur(18px);
            -webkit-backdrop-filter: blur(18px);
            border-radius: 16px;
            border: 1.5px solid var(--border-color);
            padding: 18px;
            box-shadow: 0 10px 30px rgba(2, 132, 199, 0.16), 0 2px 8px rgba(0,0,0,0.04);
            display: flex;
            flex-direction: column;
            gap: 14px;
            transition: box-shadow 0.2s, transform 0.2s;
            position: relative;
            z-index: 2;
        }
        .card:hover {
            box-shadow: 0 14px 38px rgba(2, 132, 199, 0.22);
        }
        
        .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 1.5px solid var(--border-subtle);
            padding-bottom: 10px;
        }
        
        .card-title {
            font-size: 16px;
            font-weight: 700;
            color: var(--ocean-deep);
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
            background: rgba(255, 255, 255, 0.72);
            border: 1px solid var(--border-subtle);
            padding: 8px 10px;
            border-radius: 10px;
            display: flex;
            flex-direction: column;
            gap: 2px;
            min-width: 0;
            box-shadow: 0 2px 5px rgba(0, 0, 0, 0.03);
        }
        [data-theme="dark"] .stat-block {
            background: rgba(14, 30, 54, 0.78);
            border-color: rgba(56, 189, 248, 0.22);
            box-shadow: 0 3px 10px rgba(0, 0, 0, 0.35);
        }
        
        .stat-block .stat-label {
            font-size: 10px;
            color: var(--ocean-deep);
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
        }
        [data-theme="dark"] .stat-block .stat-label {
            color: #38bdf8;
            letter-spacing: 0.6px;
        }
        
        .stat-block .stat-val {
            font-size: 13px;
            font-weight: 700;
            color: var(--text-primary);
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
        }
        [data-theme="dark"] .stat-block .stat-val {
            color: #f8fafc;
        }

        /* 24-Hour Multi-Relay Timeline Component */
        .timeline-card {
            background: rgba(255, 255, 255, 0.6);
            border: 1.5px solid var(--border-subtle);
            border-radius: 12px;
            padding: 12px 14px;
            margin-top: 6px;
            display: flex;
            flex-direction: column;
            gap: 10px;
            box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.02);
            position: relative;
        }
        [data-theme="dark"] .timeline-card {
            background: rgba(10, 24, 43, 0.65);
            border-color: rgba(56, 189, 248, 0.18);
            box-shadow: inset 0 1px 4px rgba(0, 0, 0, 0.3);
        }
        .timeline-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            flex-wrap: wrap;
            gap: 8px;
        }
        .timeline-title {
            font-size: 13px;
            font-weight: 700;
            color: var(--ocean-deep);
            display: flex;
            align-items: center;
            gap: 6px;
        }
        .timeline-legend {
            display: flex;
            align-items: center;
            gap: 12px;
            font-size: 11px;
            color: var(--text-secondary);
            font-weight: 600;
        }
        .legend-item {
            display: flex;
            align-items: center;
            gap: 5px;
        }
        .legend-swatch {
            width: 14px;
            height: 10px;
            border-radius: 3px;
            display: inline-block;
        }
        .swatch-cont {
            background: #0284c7;
        }
        .swatch-pulse {
            background: repeating-linear-gradient(-45deg, #0284c7, #0284c7 3px, #e0f2fe 3px, #e0f2fe 6px);
            border: 1px solid rgba(2, 132, 199, 0.4);
        }
        .swatch-off {
            background: rgba(148, 163, 184, 0.25);
            border: 1px solid rgba(148, 163, 184, 0.35);
        }
        [data-theme="dark"] .swatch-off {
            background: rgba(15, 26, 44, 0.65);
            border: 1px dashed rgba(56, 189, 248, 0.25);
        }
        
        .timeline-scroll-wrapper {
            overflow-x: auto;
            -webkit-overflow-scrolling: touch;
            padding-bottom: 4px;
            margin: 0 -4px;
        }
        .timeline-scroll-wrapper::-webkit-scrollbar {
            height: 6px;
        }
        .timeline-scroll-wrapper::-webkit-scrollbar-thumb {
            background: rgba(2, 132, 199, 0.25);
            border-radius: 4px;
        }
        .timeline-scroll-content {
            min-width: 820px;
            position: relative;
        }
        .timeline-ruler {
            display: flex;
            align-items: center;
            margin-bottom: 4px;
        }
        .timeline-ruler-spacer {
            width: 140px;
            flex-shrink: 0;
            position: sticky;
            left: 0;
            z-index: 10;
            background: rgba(255, 255, 255, 0.95);
            backdrop-filter: blur(8px);
        }
        [data-theme="dark"] .timeline-ruler-spacer {
            background: rgba(13, 27, 49, 0.96);
        }
        .timeline-ruler-ticks {
            flex: 1;
            display: grid;
            grid-template-columns: repeat(24, 1fr);
            gap: 3px;
            font-size: 10px;
            font-weight: 700;
            color: var(--text-muted);
            font-family: 'JetBrains Mono', Consolas, monospace;
            text-align: center;
            padding: 0 3px;
        }
        .timeline-track-row {
            display: flex;
            align-items: center;
            position: relative;
            margin-bottom: 6px;
            border-radius: 8px;
            transition: all 0.15s;
        }
        .timeline-track-row.selected {
            background: rgba(2, 132, 199, 0.08);
            box-shadow: 0 0 0 1.5px var(--ocean-blue);
        }
        [data-theme="dark"] .timeline-track-row.selected {
            background: rgba(56, 189, 248, 0.12);
            box-shadow: 0 0 0 1.5px #38bdf8;
        }
        .timeline-relay-label {
            width: 140px;
            flex-shrink: 0;
            font-size: 11px;
            font-weight: 700;
            color: var(--text-primary);
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
            display: flex;
            align-items: center;
            position: sticky;
            left: 0;
            z-index: 10;
            background: rgba(255, 255, 255, 0.96);
            backdrop-filter: blur(8px);
            padding: 6px 10px;
            border-radius: 6px 0 0 6px;
            border: 1px solid var(--border-subtle);
            border-right: none;
            box-shadow: 2px 0 4px rgba(0,0,0,0.03);
            cursor: pointer;
            user-select: none;
        }
        .timeline-relay-label:hover {
            color: var(--ocean-blue);
        }
        [data-theme="dark"] .timeline-relay-label {
            background: rgba(13, 27, 49, 0.96);
            border-color: rgba(56, 189, 248, 0.22);
            color: #f1f5f9;
            box-shadow: 3px 0 8px rgba(0, 0, 0, 0.35);
        }
        [data-theme="dark"] .timeline-relay-label:hover {
            color: #38bdf8;
            background: rgba(18, 38, 70, 0.98);
        }
        .timeline-bar-grid {
            flex: 1;
            display: grid;
            grid-template-columns: repeat(24, 1fr);
            gap: 3px;
            height: 28px;
            background: rgba(0, 0, 0, 0.04);
            padding: 3px;
            border-radius: 0 6px 6px 0;
            border: 1px solid var(--border-subtle);
        }
        [data-theme="dark"] .timeline-bar-grid {
            background: rgba(4, 10, 18, 0.7);
            border-color: rgba(56, 189, 248, 0.16);
        }
        .timeline-hour-slot {
            border-radius: 4px;
            height: 100%;
            cursor: pointer;
            transition: transform 0.15s, opacity 0.15s;
            position: relative;
            user-select: none;
        }
        .timeline-hour-slot:hover {
            transform: scaleY(1.18);
            z-index: 5;
        }
        
        .slot-off {
            background: rgba(255, 255, 255, 0.55);
            border: 1px dashed rgba(148, 163, 184, 0.4);
        }
        [data-theme="dark"] .slot-off {
            background: rgba(15, 26, 44, 0.65);
            border: 1px dashed rgba(56, 189, 248, 0.18);
        }
        [data-theme="dark"] .slot-off:hover {
            background: rgba(30, 58, 95, 0.8);
            border-color: rgba(56, 189, 248, 0.5);
        }
        .slot-cont-1 {
            background: linear-gradient(180deg, #fbbf24, #d97706);
        }
        .slot-cont-2 {
            background: linear-gradient(180deg, #34d399, #059669);
        }
        .slot-cont-3 {
            background: linear-gradient(180deg, #38bdf8, #0284c7);
        }
        .slot-cont-4 {
            background: linear-gradient(180deg, #c084fc, #7c3aed);
        }
        
        .slot-pulse-1 {
            background: repeating-linear-gradient(-45deg, #d97706, #d97706 3px, #fef3c7 3px, #fef3c7 6px);
            border: 1px solid #d97706;
        }
        .slot-pulse-2 {
            background: repeating-linear-gradient(-45deg, #059669, #059669 3px, #d1fae5 3px, #d1fae5 6px);
            border: 1px solid #059669;
        }
        .slot-pulse-3 {
            background: repeating-linear-gradient(-45deg, #0284c7, #0284c7 3px, #e0f2fe 3px, #e0f2fe 6px);
            border: 1px solid #0284c7;
        }
        .slot-pulse-4 {
            background: repeating-linear-gradient(-45deg, #7c3aed, #7c3aed 3px, #f3e8ff 3px, #f3e8ff 6px);
            border: 1px solid #7c3aed;
        }
        
        .timeline-now-line {
            position: absolute;
            top: 0;
            bottom: 0;
            width: 2px;
            background: #ef4444;
            box-shadow: 0 0 6px #ef4444;
            pointer-events: none;
            z-index: 8;
            transition: left 0.5s ease;
        }
        .timeline-now-pin {
            position: absolute;
            top: -18px;
            left: 50%;
            transform: translateX(-50%);
            background: #ef4444;
            color: white;
            font-size: 9px;
            font-weight: 800;
            padding: 1px 5px;
            border-radius: 4px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
            white-space: nowrap;
        }

        /* Feed Mode Banner */
        .feed-banner {
            display: none;
            background: linear-gradient(135deg, rgba(254, 243, 199, 0.95), rgba(253, 230, 138, 0.9));
            border: 1.5px solid #f59e0b;
            border-radius: 14px;
            padding: 12px 16px;
            align-items: center;
            justify-content: space-between;
            gap: 12px;
            box-shadow: 0 4px 14px rgba(245, 158, 11, 0.25);
            animation: pulse-orange 2.5s infinite;
        }
        .feed-banner.show {
            display: flex;
        }
        .feed-text {
            font-size: 13px;
            font-weight: 700;
            color: #92400e;
        }
        
        /* Relays List */
        .relay-item {
            background: rgba(255, 255, 255, 0.78);
            border-radius: 12px;
            padding: 12px 14px;
            border: 1.5px solid var(--border-color);
            display: flex;
            flex-direction: column;
            gap: 8px;
            box-shadow: 0 2px 8px rgba(2, 132, 199, 0.06);
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
            color: var(--ocean-deep);
            background: rgba(2, 132, 199, 0.1);
            padding: 2px 6px;
            border-radius: 4px;
            font-weight: 600;
        }
        
        .relay-status-banner {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 8px 12px;
            border-radius: 8px;
            font-size: 12px;
            font-weight: 600;
            background: rgba(255, 255, 255, 0.85);
            border: 1px solid var(--border-subtle);
            line-height: 1.4;
            letter-spacing: 0.2px;
        }
        .relay-status-banner.status-active {
            background: #ecfdf5;
            border-color: #a7f3d0;
            color: #065f46;
        }
        .relay-status-banner.status-paused {
            background: #fffbeb;
            border-color: #fde68a;
            color: #92400e;
            animation: pulse-orange 3s infinite;
        }
        .relay-status-banner.status-idle {
            background: #f8fafc;
            border-color: #e2e8f0;
            color: #475569;
        }
        .relay-status-banner.status-warn {
            background: #fef2f2;
            border-color: #fecaca;
            color: #991b1b;
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
            background: linear-gradient(135deg, #10b981, #059669);
            color: #ffffff;
            box-shadow: 0 2px 6px rgba(16, 185, 129, 0.35);
        }

        .btn-badge-auto.inactive {
            background: rgba(0, 0, 0, 0.06);
            color: #64748b;
            border-color: rgba(0, 0, 0, 0.1);
            opacity: 0.8;
        }

        .btn-badge-auto.inactive:hover {
            opacity: 1;
            background: rgba(0, 0, 0, 0.1);
            color: var(--text-primary);
        }

        /* POWER badge button */
        .btn-badge-power.neutral {
            background: rgba(0, 0, 0, 0.06);
            color: #64748b;
            border-color: rgba(0, 0, 0, 0.1);
            opacity: 0.8;
        }

        .btn-badge-power.neutral:hover {
            opacity: 1;
            background: rgba(0, 0, 0, 0.1);
            color: var(--text-primary);
        }

        .btn-badge-power.forced-on {
            background: linear-gradient(135deg, #10b981, #059669);
            color: #ffffff;
            box-shadow: 0 2px 8px rgba(16, 185, 129, 0.4);
        }

        .btn-badge-power.forced-off {
            background: linear-gradient(135deg, #ef4444, #dc2626);
            color: #ffffff;
            box-shadow: 0 2px 8px rgba(239, 68, 68, 0.4);
        }
        
        .btn {
            background: linear-gradient(135deg, #0ea5e9, #0284c7);
            color: white;
            border: none;
            padding: 8px 14px;
            border-radius: 8px;
            font-family: inherit;
            font-weight: 700;
            cursor: pointer;
            transition: all 0.2s;
            font-size: 13px;
            box-shadow: 0 2px 6px rgba(2, 132, 199, 0.25);
        }
        
        .btn:hover {
            opacity: 0.95;
            box-shadow: 0 4px 12px rgba(2, 132, 199, 0.35);
            transform: translateY(-1px);
        }
        
        .btn:active {
            transform: translateY(1px);
        }
        
        .btn-secondary {
            background: rgba(255, 255, 255, 0.8);
            color: var(--text-primary);
            border: 1px solid var(--border-subtle);
            box-shadow: none;
        }
        .btn-secondary:hover {
            background: rgba(255, 255, 255, 0.95);
            box-shadow: 0 2px 6px rgba(0,0,0,0.06);
        }
        [data-theme="dark"] .btn-secondary {
            background: rgba(14, 30, 54, 0.85);
            color: #f8fafc;
            border-color: rgba(56, 189, 248, 0.25);
        }
        [data-theme="dark"] .btn-secondary:hover {
            background: rgba(22, 49, 88, 0.95);
            border-color: #38bdf8;
            color: #ffffff;
        }
        
        .btn-danger {
            background: linear-gradient(135deg, #ef4444, #dc2626);
            color: white;
        }
        
        .btn-warning {
            background: linear-gradient(135deg, #f59e0b, #d97706);
            color: white;
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
        
        .badge-auto { background: rgba(2, 132, 199, 0.12); color: var(--ocean-deep); border: 1px solid rgba(2, 132, 199, 0.25); }
        
        /* Theme Switcher Button in Header */
        .theme-btn {
            background: rgba(255, 255, 255, 0.28);
            border: 1.5px solid var(--border-color);
            border-radius: 9999px;
            padding: 5px 11px;
            font-size: 15px;
            cursor: pointer;
            backdrop-filter: blur(10px);
            -webkit-backdrop-filter: blur(10px);
            transition: all 0.2s;
            color: var(--text-primary);
            display: inline-flex;
            align-items: center;
            justify-content: center;
            box-shadow: 0 2px 6px rgba(0,0,0,0.06);
            outline: none;
        }
        .theme-btn:hover {
            transform: scale(1.08);
            border-color: var(--ocean-blue);
        }
        [data-theme="dark"] .theme-btn {
            background: rgba(15, 23, 42, 0.65);
            border-color: rgba(56, 189, 248, 0.35);
        }

        /* Real-Time Physical Relay State Dot on Timeline Track (1.2s Cycle: 60% Base / 15% Blip 1 / 10% Gap / 15% Blip 2) */
        .timeline-relay-dot {
            width: 9px;
            height: 9px;
            border-radius: 50%;
            display: inline-block;
            flex-shrink: 0;
            margin-right: 7px;
            transition: all 0.2s;
            box-sizing: border-box;
        }
        .timeline-relay-dot.dot-idle {
            background-color: #94a3b8;
            border: 1px solid rgba(148, 163, 184, 0.4);
            box-shadow: none;
        }
        .timeline-relay-dot.dot-active {
            background-color: #10b981;
            border: 1.5px solid #10b981;
            box-shadow: 0 0 8px rgba(16, 185, 129, 0.85);
        }

        /* Intermittent Running: Base Green (60%), then 2 White Blips */
        @keyframes pulse-running-blips {
            0%, 60% {
                background-color: #10b981;
                border: 1.5px solid #10b981;
                box-shadow: 0 0 8px rgba(16, 185, 129, 0.85);
            }
            60.1%, 75% {
                background-color: #ffffff;
                border: 1.5px solid #10b981;
                box-shadow: 0 0 7px rgba(255, 255, 255, 0.95), 0 0 10px rgba(16, 185, 129, 0.7);
            }
            75.1%, 85% {
                background-color: #10b981;
                border: 1.5px solid #10b981;
                box-shadow: 0 0 8px rgba(16, 185, 129, 0.85);
            }
            85.1%, 100% {
                background-color: #ffffff;
                border: 1.5px solid #10b981;
                box-shadow: 0 0 7px rgba(255, 255, 255, 0.95), 0 0 10px rgba(16, 185, 129, 0.7);
            }
        }
        .timeline-relay-dot.dot-pulse-running {
            animation: pulse-running-blips 1.2s infinite ease-in-out;
        }

        /* Intermittent Pause: Base White (60%), then 2 Green Blips */
        @keyframes pulse-paused-blips {
            0%, 60% {
                background-color: #ffffff;
                border: 1.5px solid #10b981;
                box-shadow: 0 0 6px rgba(16, 185, 129, 0.45);
            }
            60.1%, 75% {
                background-color: #10b981;
                border: 1.5px solid #10b981;
                box-shadow: 0 0 9px rgba(16, 185, 129, 0.95);
            }
            75.1%, 85% {
                background-color: #ffffff;
                border: 1.5px solid #10b981;
                box-shadow: 0 0 6px rgba(16, 185, 129, 0.45);
            }
            85.1%, 100% {
                background-color: #10b981;
                border: 1.5px solid #10b981;
                box-shadow: 0 0 9px rgba(16, 185, 129, 0.95);
            }
        }
        .timeline-relay-dot.dot-pulse-paused {
            animation: pulse-paused-blips 1.2s infinite ease-in-out;
        }
        
        /* Empty / Inactive Timeline Track */
        .timeline-track-row.track-empty {
            opacity: 0.68;
        }
        .timeline-track-row.track-empty .timeline-bar-grid {
            background: rgba(148, 163, 184, 0.12);
        }
        .timeline-track-row.track-empty .slot-off {
            background: rgba(148, 163, 184, 0.18);
            border-color: rgba(148, 163, 184, 0.28);
        }
        .timeline-empty-notice {
            position: absolute;
            left: 0; right: 0; top: 0; bottom: 0;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 10px;
            font-weight: 700;
            color: rgba(71, 85, 105, 0.85);
            pointer-events: none;
            letter-spacing: 0.5px;
            text-transform: uppercase;
        }
        [data-theme="dark"] .timeline-empty-notice {
            color: rgba(148, 163, 184, 0.75);
        }

        /* Presets Toolbar (Mobile & Desktop Icon Layout) */
        .preset-toolbar {
            display: flex;
            gap: 6px;
            align-items: center;
            flex-wrap: nowrap;
        }
        .btn-icon {
            width: 36px;
            height: 36px;
            padding: 0;
            font-size: 15px;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            border-radius: 8px;
            flex-shrink: 0;
            background: var(--btn-bg);
            border: 1px solid var(--border-subtle);
            cursor: pointer;
            transition: all 0.15s;
            color: var(--text-primary);
        }
        .btn-icon:hover {
            transform: translateY(-1px);
            border-color: var(--ocean-blue);
            background: #ffffff;
        }
        [data-theme="dark"] .btn-icon:hover {
            background: rgba(30, 41, 59, 0.9);
            border-color: var(--accent-primary);
        }
        .btn-icon-danger:hover {
            border-color: var(--state-danger) !important;
            background: #fee2e2 !important;
            color: #dc2626 !important;
        }
        [data-theme="dark"] .btn-icon-danger:hover {
            background: rgba(153, 27, 27, 0.4) !important;
            color: #fca5a5 !important;
        }
        .btn-icon-dirty {
            border-color: var(--state-warn) !important;
            background: #fef3c7 !important;
            box-shadow: 0 0 10px rgba(245, 158, 11, 0.6) !important;
            animation: pulse-orange 2s infinite;
        }
        [data-theme="dark"] .btn-icon-dirty {
            background: rgba(120, 53, 15, 0.6) !important;
        }

        /* Compact Pulse Timings Inline Inputs (mm:ss) */
        .pulse-compact-box {
            display: flex;
            align-items: center;
            gap: 12px;
            flex-wrap: wrap;
        }
        .pulse-input-group {
            display: inline-flex;
            align-items: center;
            gap: 6px;
        }
        .pulse-time-input {
            font-family: 'JetBrains Mono', Consolas, monospace !important;
            font-size: 13px !important;
            font-weight: 700 !important;
            text-align: center;
            padding: 6px 8px !important;
            border-radius: 8px !important;
            border: 1.5px solid var(--border-subtle) !important;
            background: var(--input-bg) !important;
            color: var(--text-primary) !important;
            width: 74px !important;
        }

        /* Chapter 2 Compact Segmented Control */
        .segmented-control {
            display: inline-flex;
            background: rgba(0, 0, 0, 0.05);
            border-radius: 8px;
            padding: 3px;
            gap: 4px;
        }
        [data-theme="dark"] .segmented-control {
            background: rgba(0, 0, 0, 0.35);
        }

        /* Outage Event History Alert Colors */
        .event-pwr-alert {
            color: var(--state-danger) !important;
            font-weight: 700 !important;
        }
        .event-wifi-alert {
            color: var(--state-warn) !important;
            font-weight: 600 !important;
        }

        /* Footer */
        .aquarium-footer {
            margin-top: 24px;
            text-align: center;
            font-size: 12px;
            color: rgba(255, 255, 255, 0.92);
            text-shadow: 0 1px 3px rgba(0, 0, 0, 0.3);
            padding: 12px 10px 24px;
            font-weight: 500;
            display: flex;
            flex-direction: column;
            gap: 4px;
            z-index: 2;
        }
        [data-theme="dark"] .aquarium-footer {
            color: #94a3b8;
            text-shadow: none;
        }


        
        /* Form controls */
        .form-group {
            display: flex;
            flex-direction: column;
            gap: 5px;
        }
        
        label {
            font-size: 11px;
            color: var(--ocean-deep);
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        input[type="number"], input[type="text"], input[type="password"], select {
            background-color: rgba(255, 255, 255, 0.85);
            border: 1.5px solid var(--border-subtle);
            padding: 9px 12px;
            border-radius: 8px;
            color: var(--text-primary);
            font-family: inherit;
            font-size: 13px;
            font-weight: 600;
            outline: none;
            transition: border-color 0.2s, box-shadow 0.2s;
            width: 100%;
        }
        select option {
            background-color: #ffffff;
            color: var(--text-primary);
        }
        [data-theme="dark"] input[type="number"], 
        [data-theme="dark"] input[type="text"], 
        [data-theme="dark"] input[type="password"], 
        [data-theme="dark"] select {
            background-color: rgba(13, 27, 49, 0.85);
            border-color: rgba(56, 189, 248, 0.25);
            color: #f8fafc;
        }
        [data-theme="dark"] select option {
            background-color: #0d1b31;
            color: #f8fafc;
        }
        
        input:focus, select:focus {
            border-color: var(--ocean-blue);
            box-shadow: 0 0 0 3px rgba(2, 132, 199, 0.15);
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

        /* Timeline Container & Items (Event History) */
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
            background: rgba(2, 132, 199, 0.25);
            border-radius: 4px;
        }
        
        .timeline-item {
            display: flex;
            align-items: flex-start;
            gap: 8px;
            padding: 8px 10px;
            background: rgba(255, 255, 255, 0.65);
            border-left: 3px solid var(--ocean-blue);
            border-radius: 6px;
            font-size: 12px;
            line-height: 1.4;
            min-width: 0;
            box-shadow: 0 1px 3px rgba(0,0,0,0.02);
        }
        [data-theme="dark"] .timeline-item {
            background: rgba(14, 30, 54, 0.75);
            border-left-color: #38bdf8;
            border-top: 1px solid rgba(56, 189, 248, 0.1);
            border-right: 1px solid rgba(56, 189, 248, 0.1);
            border-bottom: 1px solid rgba(56, 189, 248, 0.1);
        }
        
        .timeline-time {
            font-family: 'JetBrains Mono', Consolas, monospace;
            font-size: 11px;
            color: var(--ocean-deep);
            background: rgba(2, 132, 199, 0.1);
            padding: 2px 6px;
            border-radius: 4px;
            flex-shrink: 0;
            font-weight: 700;
            white-space: nowrap;
        }
        [data-theme="dark"] .timeline-time {
            color: #38bdf8;
            background: rgba(56, 189, 248, 0.15);
        }
        
        .timeline-msg {
            color: var(--text-primary);
            word-break: break-word;
            flex-grow: 1;
            min-width: 0;
            font-weight: 500;
        }
        
        /* Modals */
        .modal {
            display: none;
            position: fixed;
            top: 0; left: 0; width: 100%; height: 100%;
            background: rgba(7, 89, 133, 0.45);
            backdrop-filter: blur(10px);
            -webkit-backdrop-filter: blur(10px);
            z-index: 2000;
            justify-content: center;
            align-items: center;
            padding: 16px;
        }
        .modal.show {
            display: flex;
        }
        .modal-content {
            background: rgba(255, 255, 255, 0.95);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border-radius: 16px;
            border: 1.5px solid var(--border-color);
            padding: 22px;
            max-width: 440px;
            width: 100%;
            box-shadow: 0 15px 35px rgba(2, 132, 199, 0.25);
            display: flex;
            flex-direction: column;
            gap: 14px;
            max-height: 90vh;
            overflow-y: auto;
        }
        [data-theme="dark"] .modal-content {
            background: rgba(11, 23, 40, 0.97);
            border-color: rgba(56, 189, 248, 0.28);
            box-shadow: 0 18px 45px rgba(0, 0, 0, 0.6), 0 0 25px rgba(56, 189, 248, 0.12);
        }
        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 1.5px solid var(--border-subtle);
            padding-bottom: 8px;
        }
        .modal-header h2 {
            font-size: 17px;
            font-weight: 700;
            color: var(--ocean-deep);
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
            background-color: rgba(15, 23, 42, 0.92);
            color: #ffffff;
            border: 1px solid #38bdf8;
            padding: 10px 20px;
            border-radius: 10px;
            box-shadow: 0 10px 25px rgba(0,0,0,0.3);
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
            color: rgba(255, 255, 255, 0.9);
            text-shadow: 0 1px 3px rgba(0, 0, 0, 0.3);
            padding: 12px 0 20px;
            font-weight: 600;
        }
    </style>
</head>
<body>
    <!-- Natural Volcanic Aquasoil Substrate Bed (from user's real tank photo) -->
    <div class="aquasoil-bed" aria-hidden="true"></div>

    <!-- Floating Surface Plant Roots (Salvinia / Frogbit) -->
    <div class="floating-roots-layer" aria-hidden="true">
        <svg class="roots-svg" viewBox="0 0 1200 48" preserveAspectRatio="none">
            <path d="M0,0 Q30,42 60,0 Q90,48 120,0 Q160,36 200,0 Q240,50 280,0 Q320,32 360,0 Q400,46 440,0 Q480,35 520,0 Q560,50 600,0 Q640,38 680,0 Q720,48 760,0 Q800,32 840,0 Q880,50 920,0 Q960,36 1000,0 Q1040,52 1080,0 Q1120,34 1160,0 Q1180,44 1200,0" fill="none" stroke="rgba(132, 204, 22, 0.45)" stroke-width="2.2" stroke-linecap="round"/>
            <path d="M15,0 Q45,30 75,0 Q110,42 145,0 Q180,28 215,0 Q260,46 305,0 Q345,26 385,0 Q425,40 465,0 Q505,30 545,0 Q585,44 625,0 Q665,32 705,0 Q745,42 785,0 Q825,28 865,0 Q905,42 945,0 Q985,30 1025,0 Q1065,46 1105,0 Q1145,30 1185,0" fill="none" stroke="rgba(163, 230, 53, 0.35)" stroke-width="1.6"/>
        </svg>
    </div>

    <!-- Swaying Lush Plant Stems (Staurogyne / Ludwigia / Vallisneria) -->
    <div class="plants-layer" aria-hidden="true">
        <svg class="plant plant-left" viewBox="0 0 160 320" fill="none">
            <path d="M20,320 Q35,210 15,120 Q5,40 25,0" stroke="#15803d" stroke-width="4" stroke-linecap="round"/>
            <path d="M15,220 Q55,200 45,175 Q20,195 18,215" fill="#22c55e" opacity="0.85"/>
            <path d="M16,160 Q-25,145 -15,120 Q12,140 15,155" fill="#16a34a" opacity="0.85"/>
            <path d="M18,100 Q60,85 50,60 Q18,80 16,95" fill="#4ade80" opacity="0.85"/>
            <path d="M16,50 Q-20,35 -10,15 Q14,35 18,48" fill="#22c55e" opacity="0.85"/>
            <path d="M25,0 Q32,-20 25,-25 Q18,-15 25,0" fill="#86efac" opacity="0.9"/>
        </svg>
        <svg class="plant plant-right" viewBox="0 0 160 320" fill="none">
            <path d="M140,320 Q125,210 145,120 Q155,40 135,0" stroke="#15803d" stroke-width="4" stroke-linecap="round"/>
            <path d="M145,220 Q105,200 115,175 Q140,195 142,215" fill="#22c55e" opacity="0.85"/>
            <path d="M144,160 Q185,145 175,120 Q148,140 145,155" fill="#16a34a" opacity="0.85"/>
            <path d="M142,100 Q100,85 110,60 Q142,80 144,95" fill="#4ade80" opacity="0.85"/>
            <path d="M144,50 Q180,35 170,15 Q146,35 142,48" fill="#22c55e" opacity="0.85"/>
        </svg>
    </div>


    <!-- Real Tank Critters (Always swimming head-first: scaleX(1)=Right, scaleX(-1)=Left) -->
    <!-- 1. Mickey Mouse Platy (Bright orange with 3 spots on tail) -->
    <div class="critter critter-mickey" title="Mickey Mouse Platy" aria-hidden="true">
        <svg width="50" height="28" viewBox="0 0 52 30" fill="none">
            <path d="M42,15 Q30,5 15,10 Q2,15 15,22 Q30,25 42,15 Z" fill="#f97316"/>
            <circle cx="36" cy="15" r="4.2" fill="#0f172a"/>
            <circle cx="33" cy="11" r="2.4" fill="#0f172a"/>
            <circle cx="33" cy="19" r="2.4" fill="#0f172a"/>
            <path d="M40,15 L50,6 L48,15 L50,24 Z" fill="rgba(251, 146, 60, 0.75)"/>
            <path d="M22,9 Q25,2 30,7 Z" fill="rgba(251, 146, 60, 0.7)"/>
            <circle cx="8" cy="13" r="2" fill="#ffffff"/>
            <circle cx="7.5" cy="13" r="1.2" fill="#0f172a"/>
        </svg>
    </div>

    <!-- 2. Male Guppy with Wavy Fan Tail -->
    <div class="critter critter-guppy" title="Male Guppy" aria-hidden="true">
        <svg width="52" height="28" viewBox="0 0 54 28" fill="none">
            <path class="guppy-tail" d="M36,14 Q48,2 52,6 Q48,14 53,22 Q46,26 36,14 Z" fill="url(#guppy-tail-grad)"/>
            <ellipse cx="22" cy="14" rx="14" ry="5.5" fill="#38bdf8"/>
            <path d="M18,9 Q25,1 32,7 Z" fill="rgba(236, 72, 153, 0.8)"/>
            <circle cx="11" cy="13" r="1.8" fill="#ffffff"/>
            <circle cx="10.5" cy="13" r="1" fill="#0f172a"/>
            <defs>
                <linearGradient id="guppy-tail-grad" x1="0" y1="0" x2="1" y2="1">
                    <stop offset="0%" stop-color="#ec4899"/>
                    <stop offset="50%" stop-color="#8b5cf6"/>
                    <stop offset="100%" stop-color="#38bdf8"/>
                </linearGradient>
            </defs>
        </svg>
    </div>

    <!-- 3. Guppy Fry School (Puiet de Guppy) -->
    <div class="critter critter-fry" title="Guppy Fry School" aria-hidden="true">
        <svg width="42" height="26" viewBox="0 0 44 28" fill="none">
            <ellipse cx="10" cy="8" rx="6" ry="2.2" fill="rgba(255,255,255,0.75)"/>
            <circle cx="6" cy="8" r="0.9" fill="#0f172a"/>
            <ellipse cx="24" cy="12" rx="5" ry="2" fill="rgba(255,255,255,0.7)"/>
            <circle cx="20.5" cy="12" r="0.8" fill="#0f172a"/>
            <ellipse cx="14" cy="18" rx="5.5" ry="2.2" fill="rgba(255,255,255,0.75)"/>
            <circle cx="10" cy="18" r="0.9" fill="#0f172a"/>
            <ellipse cx="32" cy="19" rx="5" ry="1.9" fill="rgba(255,255,255,0.65)"/>
            <circle cx="28.5" cy="19" r="0.8" fill="#0f172a"/>
        </svg>
    </div>

    <!-- 4. SAE 1 (Siamese Algae Eater with continuous black lateral stripe) -->
    <div class="critter critter-sae-1" title="Siamese Algae Eater (SAE)" aria-hidden="true">
        <svg width="58" height="19" viewBox="0 0 60 20" fill="none">
            <path d="M48,10 Q35,4 12,6 Q2,10 12,14 Q35,16 48,10 Z" fill="#cbd5e1"/>
            <path d="M2,10 L56,10" stroke="#0f172a" stroke-width="2.6" stroke-linecap="round"/>
            <path d="M48,10 L58,4 L55,10 L58,16 Z" fill="rgba(203, 213, 225, 0.6)"/>
            <circle cx="8" cy="9" r="1.6" fill="#fbbf24"/>
            <circle cx="7.5" cy="9" r="1" fill="#0f172a"/>
        </svg>
    </div>

    <!-- 5. SAE 2 (Lower water column) -->
    <div class="critter critter-sae-2" title="Siamese Algae Eater (SAE)" aria-hidden="true">
        <svg width="54" height="18" viewBox="0 0 56 18" fill="none">
            <path d="M44,9 Q32,4 10,6 Q2,9 10,12 Q32,14 44,9 Z" fill="#cbd5e1"/>
            <path d="M2,9 L52,9" stroke="#0f172a" stroke-width="2.4" stroke-linecap="round"/>
            <path d="M44,9 L54,3 L51,9 L54,15 Z" fill="rgba(203, 213, 225, 0.6)"/>
            <circle cx="7" cy="8" r="1.5" fill="#fbbf24"/>
            <circle cx="6.5" cy="8" r="0.9" fill="#0f172a"/>
        </svg>
    </div>

    <!-- 6. Red Cherry Shrimp (near bottom) -->
    <div class="critter critter-shrimp" title="Red Cherry Shrimp" aria-hidden="true">
        <svg width="34" height="22" viewBox="0 0 34 22" fill="none">
            <path d="M8,10 Q14,4 22,8 Q26,12 28,16" stroke="#ef4444" stroke-width="2.8" stroke-linecap="round" fill="none"/>
            <circle cx="7" cy="11" r="3" fill="#dc2626"/>
            <circle cx="5" cy="10" r="0.9" fill="#ffffff"/>
            <path d="M5,10 Q-3,4 -8,2" stroke="rgba(255,255,255,0.7)" stroke-width="0.8" fill="none"/>
            <path d="M5,12 Q-2,14 -6,16" stroke="rgba(255,255,255,0.7)" stroke-width="0.8" fill="none"/>
            <path d="M12,12 L10,17 M16,13 L15,18 M20,14 L19,19" stroke="#ef4444" stroke-width="1.2" stroke-linecap="round"/>
        </svg>
    </div>

    <!-- 7. Zebra Snail (Neritina on rock/substrate) -->
    <div class="critter critter-snail" title="Zebra Snail (Neritina)" aria-hidden="true">
        <svg width="28" height="20" viewBox="0 0 28 20" fill="none">
            <ellipse cx="14" cy="11" rx="12" ry="8" fill="#eab308"/>
            <path d="M6,6 Q13,11 7,16" stroke="#0f172a" stroke-width="2" fill="none"/>
            <path d="M12,4 Q18,11 13,18" stroke="#0f172a" stroke-width="2" fill="none"/>
            <path d="M18,5 Q24,11 20,17" stroke="#0f172a" stroke-width="2" fill="none"/>
            <ellipse cx="13" cy="17" rx="9" ry="2" fill="#fef08a" opacity="0.8"/>
        </svg>
    </div>

    <div class="container">
        <!-- Header with Theme Toggle -->
        <header>
            <div style="display: flex; align-items: center; gap: 12px;">
                <span style="font-size: 28px; filter: drop-shadow(0 2px 4px rgba(0,0,0,0.15));">🐠</span>
                <div>
                    <h1>Aquatlantis</h1>
                    <p style="font-size: 12px; color: var(--text-secondary); font-weight: 600;">Smart Controller • BioBox 56L</p>
                </div>
            </div>
            <div style="display: flex; align-items: center; gap: 8px;">
                <button id="theme-toggle-btn" class="theme-btn" onclick="cycleTheme()" title="Toggle Theme (Light / Dark / Auto)"></button>
            </div>
        </header>

        <!-- Feed Mode Alert Banner -->
        <div class="feed-banner" id="feed-banner">
            <div class="feed-text">
                🫧 <strong>Feed Mode Active:</strong> Air pump temporarily paused (<span id="feed-timer">10:00</span> remaining).
            </div>
            <button class="btn btn-sm btn-secondary" onclick="stopFeedMode()">Cancel</button>
        </div>

        <!-- SYSTEM OVERVIEW (TOP DIAGNOSTICS) -->
        <div class="card" style="padding: 14px 18px;">
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
                <span style="font-size: 13px; font-weight: 700; color: var(--ocean-deep); text-transform: uppercase; letter-spacing: 0.5px;">System Overview</span>
                <div class="sys-badge" id="wifi-badge" style="padding: 4px 10px; font-size: 12px;">
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
                <div class="stat-block" id="tailscale-stat-block" style="cursor: pointer;" onclick="openTailscaleRemote()" title="Remote Tunnel via Nexus Gateway (100.83.135.74:8080)">
                    <span class="stat-label">Nexus Remote</span>
                    <span class="stat-val" style="display: flex; align-items: center; gap: 6px; font-size: 12px;">
                        <span class="status-dot dot-orange" id="tailscale-dot"></span>
                        <span id="tailscale-status-text" style="font-family: 'JetBrains Mono', monospace; font-size: 11px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;">Checking...</span>
                    </span>
                </div>
            </div>
        </div>

        <!-- 1. 24H SCHEDULE & REAL-TIME STATUS -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">1. 24h Schedule & Real-Time Status</div>
                <div class="timeline-legend">
                    <span class="legend-item"><span class="legend-swatch swatch-cont"></span> Continuous</span>
                    <span class="legend-item"><span class="legend-swatch swatch-pulse"></span> Pulse (Hatched)</span>
                    <span class="legend-item"><span class="legend-swatch swatch-off"></span> Off</span>
                </div>
            </div>

            <!-- Scrollable 24-Hour 4-Track Timeline -->
            <div class="timeline-scroll-wrapper" id="timeline-scroll-box">
                <div class="timeline-scroll-content">
                    <div class="timeline-ruler">
                        <div class="timeline-ruler-spacer"></div>
                        <div class="timeline-ruler-ticks" id="timeline-ruler-ticks">
                            <!-- 24 ticks generated dynamically (00 to 23) -->
                        </div>
                    </div>
                    <div style="position: relative;">
                        <div id="timeline-tracks" style="display: flex; flex-direction: column;">
                            <!-- Populated dynamically via renderTimeline() -->
                        </div>
                        <div id="timeline-now-cursor" class="timeline-now-line" style="display: none;">
                            <div class="timeline-now-pin" id="timeline-now-pin">12:41</div>
                        </div>
                    </div>
                </div>
            </div>
            
            <p style="font-size: 11px; color: var(--text-muted); margin: -4px 0 2px;">💡 Tip: Tap any hour block to toggle ON / OFF. Click any channel name or row to configure its pulse schedule.</p>

            <!-- Active Channel Mode & Pulse Settings -->
            <div style="border-top: 1.5px solid var(--border-subtle); padding-top: 12px; margin-top: 6px;">
                <div id="channel-editor-prompt" style="font-size: 12px; color: var(--text-muted); padding: 4px 0; display: block;">
                    💡 Click any channel row or name above to configure its mode and pulse timing.
                </div>
                <div id="channel-editor-box" style="display: none;">
                    <div style="margin-bottom: 8px; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 6px;">
                        <span id="selected-channel-label" style="font-size: 12px; font-weight: 700; color: var(--ocean-deep);">Editing Channel: 1. LUMINA</span>
                        <button type="button" class="btn btn-sm btn-secondary" style="padding: 2px 8px; font-size: 11px;" onclick="selectRelayTrack(selectedRelayNum)" title="Close channel settings">✖ Deselect</button>
                    </div>
                    <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 12px; align-items: start;">
                        <div class="form-group">
                            <label for="relay-behavior">Mode During Active Hours</label>
                            <select id="relay-behavior" onchange="onBehaviorChanged(this.value); markScheduleModified();">
                                <option value="0">Continuous (ON)</option>
                                <option value="1">Intermittent / Pulse</option>
                            </select>
                            <small id="behavior-help-text" style="font-size: 11px; color: var(--text-muted); margin-top: 3px;">Relay stays energized throughout selected hours.</small>
                        </div>
                        
                        <div id="pulse-settings-row" style="display: none;">
                            <label style="margin-bottom: 5px; display: block;">Pulse Cycle (mm:ss)</label>
                            <div class="pulse-compact-box">
                                <div class="pulse-input-group">
                                    <span style="font-size: 11px; font-weight: 700; color: var(--text-secondary);">⏱️ ON:</span>
                                    <input type="text" id="pulse-on-time" class="pulse-time-input" placeholder="01:00" value="01:00" maxlength="5" onchange="onPulseTimeFormattedChanged()" title="Running duration (minutes:seconds, e.g. 05:00)">
                                </div>
                                <div class="pulse-input-group">
                                    <span style="font-size: 11px; font-weight: 700; color: var(--text-secondary);">⏸️ OFF:</span>
                                    <input type="text" id="pulse-off-time" class="pulse-time-input" placeholder="02:00" value="02:00" maxlength="5" onchange="onPulseTimeFormattedChanged()" title="Rest pause duration (minutes:seconds, e.g. 25:00)">
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Presets Management -->
            <div style="border-top: 1.5px solid var(--border-subtle); padding-top: 12px; margin-top: 8px; display: flex; flex-direction: column; gap: 8px;">
                <label>Schedule Presets</label>
                <div style="display: flex; gap: 8px; flex-wrap: wrap; align-items: center;">
                    <select id="preset-select" style="flex: 1; min-width: 140px;" onchange="onPresetSelected(this.value)">
                        <!-- Populated dynamically via API -->
                    </select>
                    <div class="preset-toolbar">
                        <button class="btn-icon" id="btn-save-preset" onclick="saveCurrentPreset()" title="Save Schedule to Preset">💾</button>
                        <button class="btn-icon" id="btn-restore-preset" style="display: none;" onclick="restoreSchedule()" title="Restore / Undo Unsaved Changes">↺</button>
                        <button class="btn-icon" id="btn-rename-preset" style="display: none;" onclick="openRenamePresetModal()" title="Rename Preset">✏️</button>
                        <button class="btn-icon" id="btn-new-preset" onclick="openNewPresetModal()" title="New Preset">➕</button>
                        <button class="btn-icon btn-icon-danger" id="btn-delete-preset" style="display: none;" onclick="openDeletePresetModal()" title="Delete Preset">🗑️</button>
                    </div>
                </div>
            </div>
        </div>

        <!-- 2. MANUAL CONTROL -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">2. Manual Control</div>
                <div class="segmented-control">
                    <button class="btn btn-sm btn-warning" id="btn-feed-mode" onclick="toggleFeedMode()" title="Feed Mode: Pause air pump for 10 minutes">🍽️ Feed Mode</button>
                    <button class="btn btn-secondary btn-sm" onclick="clearOverrides()" title="Return all relays to auto schedule">⚡ Auto All</button>
                </div>
            </div>
            <div id="relays-container" style="display: flex; flex-direction: column; gap: 10px;">
                <!-- Dynamically populated from status -->
            </div>
        </div>

        <!-- 3. DIGITAL INPUTS & SENSORS (I/O) -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">3. Digital Inputs & Sensors</div>
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

        <!-- 4. EVENT HISTORY (TIMELINE) -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">4. Event History</div>
                <button class="btn btn-secondary btn-sm" onclick="fetchHistory()">Refresh</button>
            </div>
            <div class="timeline-container" id="timeline-container">
                <p style="font-size: 13px; color: var(--text-secondary)">Loading event history...</p>
            </div>
        </div>

        <!-- 5. SYSTEM ADMINISTRATION -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">5. System Administration</div>
            </div>
            <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 8px;">
                <button class="btn btn-secondary" onclick="openNamesModal()">✏️ Customize I/O</button>
                <button class="btn btn-secondary" onclick="openWifiModal()">📶 WiFi Setup</button>
                <button class="btn" onclick="window.open('/update', '_blank')">⬆️ Firmware Update</button>
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

        <!-- Modal Create New Preset -->
        <div id="save-preset-modal" class="modal">
            <div class="modal-content" style="max-width: 380px;">
                <div class="modal-header">
                    <h2>Create New Preset</h2>
                    <span class="close-btn" onclick="closeNewPresetModal()">&times;</span>
                </div>
                <div class="form-group" style="margin-top: 10px;">
                    <label for="new-preset-name">Preset Name</label>
                    <input type="text" id="new-preset-name" placeholder="e.g. Summer Schedule, Treatment..." maxlength="31">
                </div>
                <div style="display: flex; gap: 8px; justify-content: flex-end; margin-top: 12px;">
                    <button class="btn btn-sm btn-secondary" onclick="closeNewPresetModal()">Cancel</button>
                    <button class="btn btn-sm" id="btn-confirm-save-preset" onclick="confirmSaveNewPreset()">Save Preset</button>
                </div>
            </div>
        </div>

        <!-- Modal Rename Preset -->
        <div id="rename-preset-modal" class="modal">
            <div class="modal-content" style="max-width: 380px;">
                <div class="modal-header">
                    <h2>Rename Preset</h2>
                    <span class="close-btn" onclick="closeRenamePresetModal()">&times;</span>
                </div>
                <div class="form-group" style="margin-top: 10px;">
                    <label for="rename-preset-name">New Preset Name</label>
                    <input type="text" id="rename-preset-name" maxlength="31">
                </div>
                <div style="display: flex; gap: 8px; justify-content: flex-end; margin-top: 12px;">
                    <button class="btn btn-sm btn-secondary" onclick="closeRenamePresetModal()">Cancel</button>
                    <button class="btn btn-sm" onclick="confirmRenamePreset()">Save Name</button>
                </div>
            </div>
        </div>

        <!-- Modal Delete Preset Confirmation -->
        <div id="delete-preset-modal" class="modal">
            <div class="modal-content" style="max-width: 360px;">
                <div class="modal-header">
                    <h2>Delete Preset</h2>
                    <span class="close-btn" onclick="closeDeletePresetModal()">&times;</span>
                </div>
                <p style="font-size: 13px; color: var(--text-secondary); margin-top: 8px;">
                    Are you sure you want to permanently delete preset <strong id="delete-preset-name-lbl"></strong>?
                </p>
                <div style="display: flex; gap: 8px; justify-content: flex-end; margin-top: 14px;">
                    <button class="btn btn-sm btn-secondary" onclick="closeDeletePresetModal()">Cancel</button>
                    <button class="btn btn-sm btn-danger" onclick="confirmDeletePreset()">Delete</button>
                </div>
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

        <!-- Footer -->
        <footer class="aquarium-footer">
            <div style="font-weight: 700; font-size: 13px; color: var(--text-primary);">🐠 Aquatlantis Smart Aquarium Controller</div>
            <div style="font-size: 11px; color: var(--text-muted);">Designed &amp; Implemented by <strong>Bogdan S.</strong> • Firmware v3.4.1</div>
        </footer>
    </div>

    <!-- Dynamic Multi-Stream Animated Bubbles (Micro, Small, Medium, Large) -->
    <div class="bubbles-layer" aria-hidden="true">
        <!-- Diffuser / Airstone Stream (Left) -->
        <div class="bubble bubble-fast" style="width: 4px; height: 4px; left: 13%; animation-duration: 6.8s; animation-delay: -2s;"></div>
        <div class="bubble bubble-wobble-1" style="width: 8px; height: 8px; left: 15%; animation-duration: 11s; animation-delay: -6s;"></div>
        <div class="bubble bubble-fast" style="width: 5px; height: 5px; left: 14%; animation-duration: 6.2s; animation-delay: -4s;"></div>
        <div class="bubble bubble-wobble-2" style="width: 12px; height: 12px; left: 16%; animation-duration: 13s; animation-delay: -9s;"></div>
        <!-- Ambient Left -->
        <div class="bubble bubble-wobble-1" style="width: 22px; height: 22px; left: 6%; animation-duration: 18s; animation-delay: -11s;"></div>
        <div class="bubble bubble-fast" style="width: 5px; height: 5px; left: 9%; animation-duration: 8s; animation-delay: -4s;"></div>
        <div class="bubble bubble-wobble-2" style="width: 16px; height: 16px; left: 22%; animation-duration: 14s; animation-delay: -6s;"></div>
        <div class="bubble bubble-fast" style="width: 3px; height: 3px; left: 26%; animation-duration: 6.2s; animation-delay: -2.5s;"></div>
        <div class="bubble bubble-wobble-1" style="width: 10px; height: 10px; left: 33%; animation-duration: 12s; animation-delay: -8.5s;"></div>
        <div class="bubble bubble-wobble-2" style="width: 26px; height: 26px; left: 38%; animation-duration: 21s; animation-delay: -14s;"></div>
        <div class="bubble bubble-fast" style="width: 4px; height: 4px; left: 42%; animation-duration: 7s; animation-delay: -5s;"></div>
        <!-- Center Column Stream -->
        <div class="bubble bubble-wobble-1" style="width: 6px; height: 6px; left: 47%; animation-duration: 10s; animation-delay: -3s;"></div>
        <div class="bubble bubble-fast" style="width: 4px; height: 4px; left: 49%; animation-duration: 7.2s; animation-delay: -1s;"></div>
        <div class="bubble bubble-wobble-2" style="width: 11px; height: 11px; left: 50%; animation-duration: 12s; animation-delay: -7s;"></div>
        <!-- Ambient Right -->
        <div class="bubble bubble-wobble-1" style="width: 18px; height: 18px; left: 58%; animation-duration: 15s; animation-delay: -9s;"></div>
        <div class="bubble bubble-fast" style="width: 3px; height: 3px; left: 64%; animation-duration: 6.5s; animation-delay: -3s;"></div>
        <div class="bubble bubble-wobble-2" style="width: 13px; height: 13px; left: 69%; animation-duration: 13s; animation-delay: -7.5s;"></div>
        <div class="bubble bubble-wobble-1" style="width: 24px; height: 24px; left: 74%; animation-duration: 19s; animation-delay: -12s;"></div>
        <div class="bubble bubble-fast" style="width: 5px; height: 5px; left: 78%; animation-duration: 7.8s; animation-delay: -4.5s;"></div>
        <!-- Filter Outflow Stream (Right) -->
        <div class="bubble bubble-fast" style="width: 4px; height: 4px; left: 81%; animation-duration: 6.8s; animation-delay: -1.5s;"></div>
        <div class="bubble bubble-wobble-1" style="width: 14px; height: 14px; left: 82%; animation-duration: 12.5s; animation-delay: -8s;"></div>
        <div class="bubble bubble-wobble-2" style="width: 9px; height: 9px; left: 83%; animation-duration: 10.5s; animation-delay: -5s;"></div>
        <div class="bubble bubble-fast" style="width: 3px; height: 3px; left: 85%; animation-duration: 6s; animation-delay: -3.5s;"></div>
        <!-- Ambient Far Right -->
        <div class="bubble bubble-wobble-2" style="width: 8px; height: 8px; left: 91%; animation-duration: 11s; animation-delay: -6s;"></div>
        <div class="bubble bubble-fast" style="width: 4px; height: 4px; left: 95%; animation-duration: 6.8s; animation-delay: -2s;"></div>
        <div class="bubble bubble-wobble-1" style="width: 20px; height: 20px; left: 97%; animation-duration: 16s; animation-delay: -10s;"></div>
    </div>

    <div class="toast" id="toast">Notification</div>

    <script>
        let schedules = [];
        let presetsList = [];
        let currentNames = null;
        let activePresetId = 1;
        let basePresetId = 1;
        let basePresetName = "";
        let selectedRelayNum = 1;
        let isScheduleDirty = false;
        let isFeedActive = false;
        let currentTheme = localStorage.getItem('aquarium_theme') || 'auto';
        let lastKnownMode = 'Day';

        function applyTheme(theme) {
            const root = document.documentElement;
            let effective = theme;
            if (theme === 'auto') {
                effective = (lastKnownMode === 'Night') ? 'dark' : 'light';
            }
            root.setAttribute('data-theme', effective);
            
            const btn = document.getElementById('theme-toggle-btn');
            if (btn) {
                if (theme === 'light') {
                    btn.innerText = '☀️ Light';
                    btn.title = 'Current Theme: Light (Click to switch to Dark)';
                } else if (theme === 'dark') {
                    btn.innerText = '🌙 Dark';
                    btn.title = 'Current Theme: Dark (Click to switch to Auto)';
                } else {
                    btn.innerText = (effective === 'dark') ? '🌓 Auto (Night)' : '🌓 Auto (Day)';
                    btn.title = 'Current Theme: Auto (Syncs with Day/Night cycle. Click to switch to Light)';
                }
            }
        }

        function cycleTheme() {
            if (currentTheme === 'auto') currentTheme = 'light';
            else if (currentTheme === 'light') currentTheme = 'dark';
            else currentTheme = 'auto';
            
            localStorage.setItem('aquarium_theme', currentTheme);
            applyTheme(currentTheme);
            showToast(`Theme switched to: ${currentTheme.toUpperCase()}`);
        }

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

        async function selectRelayTrack(num) {
            if (isScheduleDirty) {
                await saveActiveRelayScheduleSilently();
            }
            const n = parseInt(num);
            if (selectedRelayNum === n) {
                // Click again to deselect
                selectedRelayNum = 0;
            } else {
                selectedRelayNum = n;
            }
            document.querySelectorAll('.timeline-track-row').forEach(row => {
                if (parseInt(row.dataset.relay) === selectedRelayNum) row.classList.add('selected');
                else row.classList.remove('selected');
            });
            updateChannelEditorUI();
        }
        function selectRelayTab(num) { return selectRelayTrack(num); }

        function formatMMSS(sec) {
            const m = Math.floor(sec / 60);
            const s = sec % 60;
            return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
        }

        function parseMMSS(str) {
            if (!str) return 60;
            const parts = str.trim().split(':');
            if (parts.length === 1) {
                const val = parseInt(parts[0], 10);
                return isNaN(val) ? 60 : Math.max(1, val);
            }
            const m = parseInt(parts[0], 10) || 0;
            const s = parseInt(parts[1], 10) || 0;
            return Math.max(1, m * 60 + s);
        }

        function updateChannelEditorUI() {
            const promptEl = document.getElementById('channel-editor-prompt');
            const boxEl = document.getElementById('channel-editor-box');
            if (selectedRelayNum === 0) {
                if (promptEl) promptEl.style.display = 'block';
                if (boxEl) boxEl.style.display = 'none';
                return;
            }
            if (promptEl) promptEl.style.display = 'none';
            if (boxEl) boxEl.style.display = 'block';

            const sched = schedules.find(s => s.num == selectedRelayNum);
            if (!sched) return;

            const defaultNames = ["Main Light", "CO2 Solenoid", "Air Pump", "Aux Relay"];
            const customName = (currentNames && currentNames.relays && currentNames.relays[selectedRelayNum - 1]) 
                ? currentNames.relays[selectedRelayNum - 1] 
                : defaultNames[selectedRelayNum - 1];
            const gpioPin = selectedRelayNum === 1 ? 16 : selectedRelayNum === 2 ? 14 : selectedRelayNum === 3 ? 12 : 13;

            const lbl = document.getElementById('selected-channel-label');
            if (lbl) lbl.innerHTML = `Editing Channel: <strong>${selectedRelayNum}. ${customName}</strong> (GPIO ${gpioPin})`;

            const behEl = document.getElementById('relay-behavior');
            if (behEl) behEl.value = sched.behavior;
            onBehaviorChanged(sched.behavior);

            const pOn = document.getElementById('pulse-on-time');
            const pOff = document.getElementById('pulse-off-time');
            if (pOn) pOn.value = formatMMSS(sched.pulse_on);
            if (pOff) pOff.value = formatMMSS(sched.pulse_off);
        }

        function onBehaviorChanged(val) {
            const row = document.getElementById('pulse-settings-row');
            if (row) {
                row.style.display = (val == "1") ? 'block' : 'none';
            }
            const helpText = document.getElementById('behavior-help-text');
            if (helpText) {
                helpText.innerText = (val == "1") 
                    ? "Relay cycles between running and resting in active hours." 
                    : "Relay stays energized throughout selected hours.";
            }
            if (selectedRelayNum === 0) return;
            const sched = schedules.find(s => s.num == selectedRelayNum);
            if (sched && sched.behavior != val) {
                sched.behavior = parseInt(val);
                markScheduleModified();
                renderTimeline();
                saveActiveRelayScheduleSilently();
            }
        }

        function onPulseTimeFormattedChanged() {
            if (selectedRelayNum === 0) return;
            const sched = schedules.find(s => s.num == selectedRelayNum);
            if (!sched) return;
            const onInput = document.getElementById('pulse-on-time');
            const offInput = document.getElementById('pulse-off-time');
            if (!onInput || !offInput) return;
            
            sched.pulse_on = parseMMSS(onInput.value);
            sched.pulse_off = parseMMSS(offInput.value);
            
            onInput.value = formatMMSS(sched.pulse_on);
            offInput.value = formatMMSS(sched.pulse_off);
            
            markScheduleModified();
            renderTimeline();
            saveActiveRelayScheduleSilently();
        }

        function updateSaveButtonIndicator() {
            const btnSave = document.getElementById('btn-save-preset');
            const btnRestore = document.getElementById('btn-restore-preset');
            if (btnSave) {
                if (isScheduleDirty) {
                    btnSave.classList.add('btn-icon-dirty');
                    btnSave.title = "Save Schedule to Preset * (Unsaved changes)";
                } else {
                    btnSave.classList.remove('btn-icon-dirty');
                    btnSave.title = "Save Schedule to Preset";
                }
            }
            if (btnRestore) {
                btnRestore.style.display = isScheduleDirty ? 'inline-flex' : 'none';
            }
        }

        async function restoreSchedule() {
            if (!isScheduleDirty) {
                showToast("No unsaved changes to restore.");
                return;
            }
            showToast("Reverting unsaved changes...");
            await fetchSchedules();
            isScheduleDirty = false;
            updateSaveButtonIndicator();
            showToast("Schedule restored from preset!");
        }

        function markScheduleModified() {
            isScheduleDirty = true;
            updateSaveButtonIndicator();
            renderTimeline();
        }

        async function saveActiveRelayScheduleSilently() {
            if (selectedRelayNum === 0) return;
            const relayNum = selectedRelayNum;
            const sched = schedules.find(s => s.num == relayNum);
            if (!sched) return;
            const behavior = sched.behavior;
            const pulseOn = sched.pulse_on;
            const pulseOff = sched.pulse_off;
            const bitmap = sched.active_hours;

            try {
                await fetch('/api/schedule', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `relay=${relayNum}&active_hours=${bitmap}&behavior=${behavior}&pulse_on=${pulseOn}&pulse_off=${pulseOff}`
                });
                renderTimeline();
            } catch (err) {
                console.error("Silent schedule commit error:", err);
            }
        }

        async function saveAllRelaySchedulesSilently() {
            for (let r = 1; r <= 4; r++) {
                const sched = schedules.find(s => s.num == r);
                if (!sched) continue;
                try {
                    await fetch('/api/schedule', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                        body: `relay=${r}&active_hours=${sched.active_hours}&behavior=${sched.behavior}&pulse_on=${sched.pulse_on}&pulse_off=${sched.pulse_off}`
                    });
                } catch (e) {
                    console.error("Error committing relay", r, e);
                }
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
                    if (/^\d{4}[-/]\d{2}[-/]\d{2}/.test(t)) {
                        const parts = t.split(' ');
                        const dp = parts[0].split(/[-/]/);
                        t = `${dp[2]}.${dp[1]}${parts[1] ? ' ' + parts[1] : ''}`;
                    }
                    let msgClass = '';
                    if (event.msg.includes('[PWR]') || event.msg.includes('Power Outage') || event.msg.includes('Power Restored')) {
                        msgClass = 'event-pwr-alert';
                    } else if (event.msg.includes('[WIFI]') || event.msg.includes('WiFi')) {
                        msgClass = 'event-wifi-alert';
                    }

                    const item = document.createElement('div');
                    item.className = 'timeline-item';
                    item.innerHTML = `
                        <span class="timeline-time">${t}</span>
                        <span class="timeline-msg ${msgClass}">${event.msg}</span>
                    `;
                    container.appendChild(item);
                });
            } catch (err) {
                console.error("Error fetching history:", err);
            }
        }

        let hasAutoScrolled = false;
        function autoScrollTimeline() {
            if (hasAutoScrolled) return;
            const scrollBox = document.getElementById('timeline-scroll-box');
            const firstGrid = document.querySelector('.timeline-bar-grid');
            if (!scrollBox || !firstGrid) return;
            const now = new Date();
            const hr = now.getHours();
            const slotWidth = firstGrid.offsetWidth / 24;
            const target = Math.max(0, firstGrid.offsetLeft + (hr * slotWidth) - (scrollBox.clientWidth / 2));
            scrollBox.scrollLeft = target;
            hasAutoScrolled = true;
        }

        async function fetchSchedules() {
            try {
                const res = await fetch('/api/schedule');
                schedules = await res.json();
                updateChannelEditorUI();
                renderTimeline();
                autoScrollTimeline();
            } catch (err) {
                console.error("Error fetching schedules:", err);
            }
        }

        function updateTimelineDot(relay) {
            if (!relay) return;
            const dot = document.getElementById(`dot-relay-${relay.num}`);
            if (!dot) return;

            let isHourActive = false;
            let isPulseMode = false;
            if (schedules) {
                const sched = schedules.find(s => s.num == relay.num);
                if (sched) {
                    isPulseMode = (sched.behavior == 1);
                    const timeStr = document.getElementById('time-val')?.innerText;
                    if (timeStr && timeStr !== '-' && timeStr !== '00:00:00') {
                        const hr = parseInt(timeStr.split(':')[0], 10);
                        if (!isNaN(hr)) {
                            isHourActive = (sched.active_hours & (1 << hr)) !== 0;
                        }
                    }
                }
            }

            if (relay.state) {
                if (isPulseMode && isHourActive && !relay.override) {
                    dot.className = 'timeline-relay-dot dot-pulse-running';
                    dot.title = `${relay.name}: Pulse ON (Running • 2 White Blips)`;
                } else {
                    dot.className = 'timeline-relay-dot dot-active';
                    dot.title = `${relay.name}: Continuous ON (Conducting)`;
                }
            } else if (relay.in_pause || (isPulseMode && isHourActive && !relay.override)) {
                dot.className = 'timeline-relay-dot dot-pulse-paused';
                dot.title = `${relay.name}: Pulse PAUSE (Standby • 2 Green Blips)`;
            } else {
                dot.className = 'timeline-relay-dot dot-idle';
                dot.title = `${relay.name}: Inactive (Off)`;
            }
        }

        let lastRelayStates = null;
        function renderTimeline() {
            const container = document.getElementById('timeline-tracks');
            if (!container || !schedules || schedules.length === 0) return;
            container.innerHTML = '';

            const ticksEl = document.getElementById('timeline-ruler-ticks');
            if (ticksEl && ticksEl.children.length === 0) {
                ticksEl.innerHTML = '';
                for (let h = 0; h < 24; h++) {
                    const s = document.createElement('span');
                    s.innerText = h.toString().padStart(2, '0');
                    ticksEl.appendChild(s);
                }
            }

            const defaultNames = ["Iluminat Principal", "Electrovalva CO2", "Pompa de Aer", "Liber"];

            for (let i = 0; i < 4; i++) {
                const relayNum = i + 1;
                const sched = schedules.find(s => s.num == relayNum) || { active_hours: 0, behavior: 0, pulse_on: 60, pulse_off: 120 };
                const customName = (currentNames && currentNames.relays && currentNames.relays[i]) ? currentNames.relays[i] : defaultNames[i];
                const isChannelEmpty = (sched.active_hours === 0);
                
                const trackRow = document.createElement('div');
                trackRow.className = 'timeline-track-row' + (relayNum === selectedRelayNum ? ' selected' : '') + (isChannelEmpty ? ' track-empty' : '');
                trackRow.dataset.relay = relayNum;
                trackRow.onclick = () => selectRelayTrack(relayNum);

                const label = document.createElement('div');
                label.className = 'timeline-relay-label';
                label.title = `${customName} (Click to select channel)`;
                const emptyBadge = isChannelEmpty ? ' <span style="font-size:9px;color:var(--text-muted);font-weight:700;margin-left:auto;padding-right:2px;">[OFF]</span>' : '';
                label.innerHTML = `
                    <span class="timeline-relay-dot dot-idle" id="dot-relay-${relayNum}"></span>
                    <span style="overflow:hidden;text-overflow:ellipsis;">${customName}</span>${emptyBadge}
                `;

                const barGrid = document.createElement('div');
                barGrid.className = 'timeline-bar-grid';
                barGrid.style.position = 'relative';

                for (let h = 0; h < 24; h++) {
                    const slot = document.createElement('div');
                    slot.className = 'timeline-hour-slot';
                    const isActive = (sched.active_hours & (1 << h)) !== 0;

                    if (isActive) {
                        if (sched.behavior == 1) {
                            slot.classList.add(`slot-pulse-${relayNum}`);
                            const onM = Math.floor(sched.pulse_on / 60);
                            const onS = sched.pulse_on % 60;
                            const offM = Math.floor(sched.pulse_off / 60);
                            const offS = sched.pulse_off % 60;
                            const onStr = onM > 0 ? (onS > 0 ? `${onM}m${onS}s` : `${onM}m`) : `${onS}s`;
                            const offStr = offM > 0 ? (offS > 0 ? `${offM}m${offS}s` : `${offM}m`) : `${offS}s`;
                            slot.title = `${customName} • Hour ${h.toString().padStart(2, '0')}:00\nPulse: ${onStr} ON / ${offStr} OFF\n(Tap to toggle OFF)`;
                        } else {
                            slot.classList.add(`slot-cont-${relayNum}`);
                            slot.title = `${customName} • Hour ${h.toString().padStart(2, '0')}:00\nContinuous ON\n(Tap to toggle OFF)`;
                        }
                    } else {
                        slot.classList.add('slot-off');
                        slot.title = `${customName} • Hour ${h.toString().padStart(2, '0')}:00\nOFF\n(Tap to toggle ON)`;
                    }

                    // Tapping slot toggles this hour directly
                    slot.onclick = async (e) => {
                        e.stopPropagation();
                        if (selectedRelayNum !== relayNum) {
                            await selectRelayTrack(relayNum);
                        }
                        sched.active_hours ^= (1 << h);
                        markScheduleModified();
                        renderTimeline();
                        await saveActiveRelayScheduleSilently();
                    };

                    barGrid.appendChild(slot);
                }

                if (isChannelEmpty) {
                    const notice = document.createElement('div');
                    notice.className = 'timeline-empty-notice';
                    notice.innerText = '⚪ All 24h OFF • Click any hour to schedule';
                    barGrid.appendChild(notice);
                }

                trackRow.appendChild(label);
                trackRow.appendChild(barGrid);
                container.appendChild(trackRow);
            }

            if (lastRelayStates) {
                lastRelayStates.forEach(relay => updateTimelineDot(relay));
            }

            const timeStr = document.getElementById('time-val')?.innerText;
            if (timeStr) updateTimelineNowCursor(timeStr);
        }

        function updateTimelineNowCursor(timeStr) {
            const cursor = document.getElementById('timeline-now-cursor');
            const pin = document.getElementById('timeline-now-pin');
            if (!cursor || !pin || !timeStr || timeStr === '00:00:00' || timeStr === '-') return;

            const firstGrid = document.querySelector('.timeline-bar-grid');
            if (!firstGrid) {
                cursor.style.display = 'none';
                return;
            }

            const parts = timeStr.split(':');
            if (parts.length < 2) return;
            const hr = parseInt(parts[0], 10);
            const min = parseInt(parts[1], 10);
            const sec = parts[2] ? parseInt(parts[2], 10) : 0;
            if (isNaN(hr) || isNaN(min)) return;

            const totalSec = hr * 3600 + min * 60 + sec;
            const pct = Math.min(1.0, Math.max(0.0, totalSec / 86400));

            const gridLeft = firstGrid.offsetLeft;
            const gridWidth = firstGrid.offsetWidth;
            const leftPx = gridLeft + pct * gridWidth;

            cursor.style.left = `${leftPx}px`;
            cursor.style.display = 'block';
            pin.innerText = `${hr.toString().padStart(2, '0')}:${min.toString().padStart(2, '0')}`;
        }

        window.addEventListener('resize', () => {
            const timeStr = document.getElementById('time-val')?.innerText;
            if (timeStr) updateTimelineNowCursor(timeStr);
        });

        let lastTailscaleCheck = 0;
        let lastTailscaleState = null;
        async function updateTailscaleIndicator() {
            const dot = document.getElementById('tailscale-dot');
            const text = document.getElementById('tailscale-status-text');
            if (!dot || !text) return;

            const isTailscaleHost = (window.location.hostname === '100.83.135.74' || window.location.hostname === 'cinderella');

            if (isTailscaleHost) {
                dot.className = 'status-dot dot-green';
                text.innerText = 'Connected (Remote)';
                text.title = 'Connected directly via Tailscale (100.83.135.74:8080)';
                return;
            }

            const now = Date.now();
            if (now - lastTailscaleCheck < 8000 && lastTailscaleState !== null) {
                return;
            }
            lastTailscaleCheck = now;

            // Check if Cinderella proxy on 192.168.1.28:8080 is reachable
            try {
                const res = await fetch('http://192.168.1.28:8080/api/status', { method: 'GET', signal: AbortSignal.timeout(1500) });
                if (res.ok) {
                    dot.className = 'status-dot dot-green';
                    text.innerText = '100.83.135.74:8080';
                    text.title = 'Tailscale Bridge Active on Cinderella (Click to open remote link)';
                    lastTailscaleState = true;
                } else {
                    dot.className = 'status-dot dot-orange';
                    text.innerText = 'Standby';
                    lastTailscaleState = false;
                }
            } catch (e) {
                dot.className = 'status-dot dot-idle';
                text.innerText = 'Local Only';
                text.title = 'Tailscale bridge on 192.168.1.28:8080 not reachable';
                lastTailscaleState = false;
            }
        }

        function openTailscaleRemote() {
            window.open('http://100.83.135.74:8080/', '_blank');
        }

        async function fetchPresets() {
            try {
                const res = await fetch('/api/presets');
                presetsList = await res.json();
                const sel = document.getElementById('preset-select');
                if (!sel) return;
                sel.innerHTML = '';

                presetsList.forEach(p => {
                    const opt = document.createElement('option');
                    opt.value = p.id;
                    opt.innerText = p.name;
                    sel.appendChild(opt);
                });

                if (activePresetId && presetsList.some(p => p.id == activePresetId)) {
                    sel.value = activePresetId;
                } else if (presetsList.length > 0) {
                    activePresetId = presetsList[0].id;
                    basePresetId = presetsList[0].id;
                    sel.value = activePresetId;
                }
                const curP = presetsList.find(x => x.id == sel.value);
                const btnDel = document.getElementById('btn-delete-preset');
                const btnRename = document.getElementById('btn-rename-preset');
                if (btnDel) btnDel.style.display = (curP && !curP.builtin) ? 'inline-flex' : 'none';
                if (btnRename) btnRename.style.display = (curP && !curP.builtin) ? 'inline-flex' : 'none';
            } catch (err) {
                console.error("Error fetching presets:", err);
            }
        }

        async function onPresetSelected(val) {
            const pId = parseInt(val);
            const curP = presetsList.find(x => x.id == pId);
            const btnDel = document.getElementById('btn-delete-preset');
            const btnRename = document.getElementById('btn-rename-preset');
            if (btnDel) btnDel.style.display = (curP && !curP.builtin) ? 'inline-flex' : 'none';
            if (btnRename) btnRename.style.display = (curP && !curP.builtin) ? 'inline-flex' : 'none';

            if (isNaN(pId) || pId === 0) return;

            if (pId !== activePresetId) {
                try {
                    const res = await fetch('/api/presets', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                        body: `apply=${pId}`
                    });
                    if (res.ok) {
                        isScheduleDirty = false;
                        updateSaveButtonIndicator();
                        activePresetId = pId;
                        basePresetId = pId;
                        showToast(`Preset "${curP ? curP.name : pId}" loaded!`);
                        await fetchSchedules();
                        await fetchStatus();
                        await fetchHistory();
                    } else {
                        showToast("Error loading preset!");
                    }
                } catch (e) {
                    console.error("Error loading preset:", e);
                    showToast("Communication error!");
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
                updateTimelineNowCursor(timeStr);
                document.getElementById('uptime-val').innerText = formatUptime(data.uptime);
                document.getElementById('ssid-val').innerText = data.wifi_ssid;
                document.getElementById('rssi-val').innerText = data.wifi_rssi + ' dBm';
                document.getElementById('ip-val').innerText = data.ip;
                updateTailscaleIndicator();
                
                // Update light meter in Section 4
                const lightVal = document.getElementById('light-val');
                if (lightVal) lightVal.innerText = data.light_percent + '%';
                const lightBar = document.getElementById('light-meter-bar');
                if (lightBar) lightBar.style.width = data.light_percent + '%';
                const lightPctLbl = document.getElementById('light-percent-label');
                if (lightPctLbl) lightPctLbl.innerText = data.light_percent + '%';
                
                // Track active preset from backend if non-zero and user has no unsaved modifications
                if (!isScheduleDirty && data.active_preset && data.active_preset > 0 && data.active_preset !== activePresetId) {
                    activePresetId = data.active_preset;
                    basePresetId = data.active_preset;
                    fetchPresets();
                }

                lastKnownMode = (data.mode && data.mode.includes('Night')) ? 'Night' : 'Day';
                if (currentTheme === 'auto') {
                    applyTheme('auto');
                }

                // Feed mode banner & Chapter 2 button state
                isFeedActive = !!data.feed_mode;
                const feedBanner = document.getElementById('feed-banner');
                const btnFeed = document.getElementById('btn-feed-mode');
                if (btnFeed) {
                    if (isFeedActive) {
                        btnFeed.classList.remove('btn-warning');
                        btnFeed.classList.add('btn-danger');
                        btnFeed.innerText = '⏹️ Stop Feed';
                    } else {
                        btnFeed.classList.remove('btn-danger');
                        btnFeed.classList.add('btn-warning');
                        btnFeed.innerText = '🍽️ Feed Mode';
                    }
                }
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
                }

                // Update live physical state dots on timeline tracks & selected channel label
                if (data.relays) {
                    lastRelayStates = data.relays;
                    data.relays.forEach(relay => updateTimelineDot(relay));
                    
                    const currentRelay = data.relays.find(r => r.num === selectedRelayNum);
                    const rName = currentRelay ? currentRelay.name : (currentNames?.relays?.[selectedRelayNum - 1] || `Relay ${selectedRelayNum}`);
                    const gpioPin = selectedRelayNum === 1 ? 16 : selectedRelayNum === 2 ? 14 : selectedRelayNum === 3 ? 12 : 13;
                    const lbl = document.getElementById('selected-channel-label');
                    if (lbl) lbl.innerHTML = `Editing Channel: <strong>${selectedRelayNum}. ${rName}</strong> (GPIO ${gpioPin})`;
                }

                // WiFi status badge
                const wifiDot = document.getElementById('wifi-dot');
                const wifiText = document.getElementById('wifi-status-text');
                wifiText.innerText = data.wifi_status;
                wifiDot.className = 'status-dot';
                if (data.wifi_status === 'Connected') wifiDot.classList.add('dot-green');
                else wifiDot.classList.add('dot-red');

                // Relays card rendering (Compact interactive badges)
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
                    
                    const svgPower = '<svg style="vertical-align:middle;margin-right:2px;display:inline-block;" viewBox="0 0 24 24" width="13" height="13" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"><path d="M12 2v9M18.36 6.64A9 9 0 1 1 5.63 6.64"/></svg>';
                    let powerClass = 'btn-badge-power neutral';
                    let powerLabel = svgPower;
                    if (relay.override) {
                        if (relay.state) {
                            powerClass = 'btn-badge-power forced-on';
                            powerLabel = `${svgPower} ON`;
                        } else {
                            powerClass = 'btn-badge-power forced-off';
                            powerLabel = `${svgPower} OFF`;
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

        async function saveCurrentPreset() {
            const sel = document.getElementById('preset-select');
            const pId = parseInt(sel ? sel.value : basePresetId);
            const p = presetsList.find(x => x.id == pId);

            if (p && !p.builtin) {
                if (!confirm(`Save current schedule and overwrite preset "${p.name}"?`)) {
                    return;
                }
                await saveAllRelaySchedulesSilently();
                try {
                    const res = await fetch('/api/presets', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                        body: `overwrite=${p.id}`
                    });
                    if (res.ok) {
                        isScheduleDirty = false;
                        updateSaveButtonIndicator();
                        activePresetId = p.id;
                        basePresetId = p.id;
                        showToast(`Preset "${p.name}" saved!`);
                        await fetchSchedules();
                        await fetchPresets();
                        await fetchStatus();
                        await fetchHistory();
                    } else {
                        showToast("Error saving preset!");
                    }
                } catch (err) {
                    showToast("Communication error!");
                }
            } else {
                const suggested = p ? `${p.name} (Custom)` : 'Custom Schedule';
                openNewPresetModal(suggested);
            }
        }

        function openNewPresetModal(suggestedName = '') {
            const modal = document.getElementById('save-preset-modal');
            const input = document.getElementById('new-preset-name');
            if (modal && input) {
                input.value = suggestedName || '';
                modal.classList.add('show');
                input.focus();
            }
        }

        function closeNewPresetModal() {
            const modal = document.getElementById('save-preset-modal');
            if (modal) modal.classList.remove('show');
        }

        async function confirmSaveNewPreset() {
            const input = document.getElementById('new-preset-name');
            const name = input ? input.value.trim() : '';
            if (!name) {
                showToast("Please enter a name for the preset!");
                return;
            }

            await saveAllRelaySchedulesSilently();

            try {
                const res = await fetch('/api/presets', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `save=1&name=${encodeURIComponent(name)}`
                });
                if (res.ok) {
                    const data = await res.json();
                    isScheduleDirty = false;
                    updateSaveButtonIndicator();
                    if (data && data.id) {
                        activePresetId = data.id;
                        basePresetId = data.id;
                    }
                    showToast(`Preset "${name}" saved!`);
                    closeNewPresetModal();
                    await fetchSchedules();
                    await fetchPresets();
                    await fetchStatus();
                    await fetchHistory();
                } else {
                    showToast("Error saving preset!");
                }
            } catch (err) {
                showToast("Communication error!");
            }
        }

        function openDeletePresetModal() {
            const sel = document.getElementById('preset-select');
            const pId = parseInt(sel ? sel.value : 0);
            const p = presetsList.find(x => x.id == pId);
            if (!p || p.builtin) return;
            const modal = document.getElementById('delete-preset-modal');
            const lbl = document.getElementById('delete-preset-name-lbl');
            if (lbl) lbl.innerText = `"${p.name}"`;
            if (modal) modal.classList.add('show');
        }

        function closeDeletePresetModal() {
            const modal = document.getElementById('delete-preset-modal');
            if (modal) modal.classList.remove('show');
        }

        async function confirmDeletePreset() {
            const sel = document.getElementById('preset-select');
            const pId = parseInt(sel ? sel.value : 0);
            const p = presetsList.find(x => x.id == pId);
            if (!p || p.builtin) return;

            try {
                const res = await fetch('/api/presets', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `delete=${pId}`
                });
                if (res.ok) {
                    showToast(`Preset "${p.name}" deleted!`);
                    closeDeletePresetModal();
                    activePresetId = 1;
                    basePresetId = 1;
                    await fetchPresets();
                    await fetchSchedules();
                    await fetchHistory();
                } else {
                    showToast("Error deleting preset!");
                }
            } catch (err) {
                showToast("Communication error!");
            }
        }

        function openRenamePresetModal() {
            const sel = document.getElementById('preset-select');
            const pId = parseInt(sel ? sel.value : 0);
            const p = presetsList.find(x => x.id == pId);
            if (!p || p.builtin) return;
            const modal = document.getElementById('rename-preset-modal');
            const input = document.getElementById('rename-preset-name');
            if (modal && input) {
                input.value = p.name;
                modal.classList.add('show');
                input.focus();
            }
        }

        function closeRenamePresetModal() {
            const modal = document.getElementById('rename-preset-modal');
            if (modal) modal.classList.remove('show');
        }

        async function confirmRenamePreset() {
            const sel = document.getElementById('preset-select');
            const pId = parseInt(sel ? sel.value : 0);
            const p = presetsList.find(x => x.id == pId);
            if (!p || p.builtin) return;

            const input = document.getElementById('rename-preset-name');
            const name = input ? input.value.trim() : '';
            if (!name) {
                showToast("Please enter a name for the preset!");
                return;
            }

            try {
                const res = await fetch('/api/presets', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `rename=${pId}&name=${encodeURIComponent(name)}`
                });
                if (res.ok) {
                    showToast(`Preset renamed to "${name}"!`);
                    closeRenamePresetModal();
                    await fetchPresets();
                    await fetchHistory();
                } else {
                    showToast("Error renaming preset!");
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
                    isScheduleDirty = false;
                    activePresetId = 1;
                    showToast("Relay schedules reset to factory defaults!");
                    await fetchSchedules();
                    await fetchPresets();
                    await fetchStatus();
                    await fetchHistory();
                }
            } catch (err) {
                showToast("Communication error!");
            }
        }

        async function toggleFeedMode() {
            if (isFeedActive) {
                await stopFeedMode();
            } else {
                await startFeedMode();
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
        applyTheme(currentTheme);
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
    server.sendHeader("Access-Control-Allow-Origin", "*");
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
        r += "\"in_pause\":" + String(isRelayInPulsePause(i) ? "true" : "false") + ",";
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
    } else if (server.hasArg("overwrite")) {
        int presetNum = server.arg("overwrite").toInt();
        if (presetNum >= 100) {
            bool ok = overwriteUserPreset(presetNum);
            if (ok) {
                server.send(200, "application/json", "{\"success\":true,\"id\":" + String(presetNum) + "}");
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
    } else if (server.hasArg("rename") && server.hasArg("name")) {
        int presetNum = server.arg("rename").toInt();
        String name = server.arg("name");
        name.trim();
        if (presetNum >= 100 && name.length() > 0) {
            bool ok = renameUserPreset(presetNum, name);
            if (ok) {
                server.send(200, "application/json", "{\"success\":true,\"id\":" + String(presetNum) + "}");
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
    
    // 1. Debounce PIN_INPUT_GPIO0 (Power Sense) to avoid false triggers or flapping
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
    
    // Outage Duration Tracking: WiFi drops and Power Loss
    static bool wasInPowerLoss = false;
    static unsigned long powerLossStartMillis = 0;
    static bool wasWifiConnected = true;
    static unsigned long wifiDropStartMillis = 0;
    
    // Check WiFi connection status and measure disconnect duration
    bool isWifiConnected = (WiFi.status() == WL_CONNECTED);
    if (!isWifiConnected && wasWifiConnected) {
        wifiDropStartMillis = millis();
        wasWifiConnected = false;
        Serial.println(F("[WIFI] Connection lost! Tracking outage duration..."));
    } else if (isWifiConnected && !wasWifiConnected) {
        wasWifiConnected = true;
        if (wifiDropStartMillis > 0) {
            uint32_t durSec = (millis() - wifiDropStartMillis) / 1000UL;
            wifiDropStartMillis = 0;
            logSystemEvent("[WIFI] Connection Restored after " + formatDuration(durSec) + " outage");
        }
    }
    
    // 2. Execute state machine logic based on NTP synchronized time and Power Sense (debounced)
    if (debouncedPowerState == LOW) {
        currentSystemMode = MODE_POWER_LOSS;
        if (!wasInPowerLoss) {
            wasInPowerLoss = true;
            powerLossStartMillis = millis();
            logSystemEvent("[PWR] Power Outage Detected! Switched to Battery Failsafe");
        }
    } else {
        if (wasInPowerLoss) {
            wasInPowerLoss = false;
            uint32_t durSec = (powerLossStartMillis > 0) ? ((millis() - powerLossStartMillis) / 1000UL) : 0;
            powerLossStartMillis = 0;
            logSystemEvent("[PWR] AC Grid Restored after " + formatDuration(durSec) + " outage");
        }
        
        if (isTimeSynced()) {
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
