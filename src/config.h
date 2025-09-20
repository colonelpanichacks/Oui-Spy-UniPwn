/*
 * Configuration file for ESP32 Unitree Exploit Tool
 * Modify these settings for your specific use case
 */

#ifndef CONFIG_H
#define CONFIG_H

// Device Configuration
#define DEVICE_NAME "ESP32-UniPwn"
#define SCAN_TIME_SECONDS 30
#define CONNECTION_TIMEOUT 30000
#define NOTIFICATION_TIMEOUT 5000

// BLE Configuration
#define BLE_SCAN_INTERVAL 100
#define BLE_SCAN_WINDOW 99
#define BLE_ACTIVE_SCAN true

// Memory Settings
#define MAX_RECENT_DEVICES 5
#define CHUNK_SIZE 14
#define MAX_DEVICE_NAME_LENGTH 32
#define MAX_COMMAND_LENGTH 256

// Feature Flags
#define ENABLE_WEB_INTERFACE true   // OUI Spy style web interface enabled
#define ENABLE_DISPLAY false        // Set to true if using OLED/TFT display
#define ENABLE_SD_LOGGING false     // Set to true for SD card logging
#define ENABLE_GPS false            // Set to true for GPS location logging

// Web Interface Settings (if enabled)
#if ENABLE_WEB_INTERFACE
#define WIFI_AP_SSID "unipwn"
#define WIFI_AP_PASSWORD "unipwn123"
#define WEB_SERVER_PORT 80
#endif

// Display Settings (if enabled)
#if ENABLE_DISPLAY
#define DISPLAY_SDA 21
#define DISPLAY_SCL 22
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#endif

// SD Card Settings (if enabled)
#if ENABLE_SD_LOGGING
#define SD_CS_PIN 5
#define LOG_FILENAME "/exploit_log.txt"
#endif

// GPS Settings (if enabled)
#if ENABLE_GPS
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD_RATE 9600
#endif

// Security Settings
#define REQUIRE_PHYSICAL_BUTTON_PRESS false  // Require button press to start exploit
#define ENABLE_ENCRYPTION_LOG false         // Log encryption operations (debug)
#define MAX_FAILED_ATTEMPTS 3              // Max failed connection attempts

// Performance Tuning
#define BLE_STACK_SIZE 8192
#define MAIN_TASK_STACK_SIZE 4096
#define NOTIFICATION_QUEUE_SIZE 10

// Hardware-specific settings
#ifdef ARDUINO_ESP32S3_DEV
#define HAS_PSRAM true
#define LED_PIN 2
#endif

#ifdef ARDUINO_ESP32S3_BOX
#define HAS_PSRAM true
#define HAS_DISPLAY true
#define LED_PIN 47
#endif

#ifdef ARDUINO_XIAO_ESP32S3
#define HAS_PSRAM true
#define LED_PIN 21  // XIAO ESP32-S3 built-in LED
#undef DEVICE_NAME
#define DEVICE_NAME "XIAO-UniPwn"
#define COMPACT_MODE true  // Optimized for small form factor
#endif

// Debug Settings
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// Version Information
#define FIRMWARE_VERSION "1.0.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

#endif // CONFIG_H
