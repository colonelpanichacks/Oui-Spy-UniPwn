# Oui-Spy UniPwn Port

<p align="center">
  <img src="bot.png" alt="OUI Spy UniPwn Robot" width="400"/>
</p>



## Overview

This project ports the powerful UniPwn Unitree robot exploitation research to ESP32-S3 microcontrollers, creating a portable exploitation device with a web interface. Based on the original research by Bin4ry and d0tslash, this ESP32 implementation brings comprehensive BLE-based robot exploitation to a compact, battery-powered form factor.

## Research Credits

This implementation is built upon the groundbreaking security research conducted by:
- **Bin4ry** - Primary researcher and vulnerability discovery
- **d0tslash** - Co-researcher and exploitation development
- **Original Research**: https://github.com/Bin4ry/UniPwn

All credit for the vulnerability discovery, protocol analysis, and initial exploitation techniques goes to these security researchers. This ESP32 port simply adapts their excellent work to embedded hardware.

## Features

### Advanced Exploitation Capabilities
- **BLE Command Injection**: Exploits WiFi configuration vulnerability in Unitree robots
- **Multi-Robot Support**: Go2, G1, H1, and B2 series compatibility
- **AutoPwn**: 7-step automated exploitation with bulletproof error handling
- **Real-time BLE Scanning**: Live device discovery with signal strength indicators
- **Custom Command Injection**: Manual exploitation with SSID/password injection methods

### Web Interface
- **OUI Spy Design**: Modern glassmorphism UI with lime green to hot pink ASCII gradient
- **Target Management**: Click-to-select discovered robots with detailed information
- **Continuous Scanning**: Automated scanning with visual progress indicators
- **Attack Vectors**: Pre-configured exploits (SSH enable, root access, system info)
- **Operations Logging**: Comprehensive audit trail with timestamps and status codes
- **System Monitoring**: Real-time heap, uptime, and network status

### AutoPwn Features
- **Bulletproof Error Handling**: 3 retries per step with progressive backoff
- **Timeout Protection**: 10-second timeouts on all network operations
- **Step Validation**: Each exploitation phase verified before proceeding
- **Progress Tracking**: Real-time progress bar with detailed status updates
- **Comprehensive Logging**: Complete forensic trail of exploitation attempts

### Hardware Platform
- **ESP32-S3 Optimized**: Built for Seeed XIAO ESP32-S3 with PSRAM support
- **Wireless AP Mode**: Creates isolated "unipwn" network for interface access
- **Detailed Logging**: Comprehensive BLE scan results and exploitation status
- **Responsive UI**: Instant feedback on all user interactions

## Technical Implementation

### Supported Robot Models
- **Unitree Go2**: Latest generation quadruped robot
- **Unitree G1**: Humanoid robot platform  
- **Unitree H1**: Advanced humanoid robot
- **Unitree B2**: Industrial quadruped robot

### Exploitation Method
Based on the original UniPwn research, this tool exploits a command injection vulnerability in the WiFi configuration process. The attack uses BLE communication to deliver malicious payloads through unsanitized SSID and password fields, executing commands with system privileges.

### Key Technical Features
- **AES-CFB128 Encryption**: Proper implementation of Unitree's crypto protocol
- **BLE Protocol Stack**: Complete implementation of the 6-instruction handshake
- **Command Injection Payloads**: Multiple injection vectors with proper escaping
- **Error Recovery**: Comprehensive retry logic with detailed failure analysis

## Quick Start

### Hardware Requirements
- ESP32-S3 development board (Seeed XIAO ESP32-S3 recommended)
- USB-C cable for programming
- Unitree robot target for testing (with proper authorization)

### Installation
1. Install PlatformIO development environment
2. Clone this repository to your local machine
3. Connect ESP32-S3 board via USB-C
4. Execute: `pio run -e seeed_xiao_esp32s3 -t upload`
5. Monitor serial output for startup confirmation

### Operation
1. Power on the ESP32-S3 device
2. Connect to "unipwn" WiFi network (password: unipwn123)
3. Navigate to http://192.168.4.1 in web browser
4. Use "Start BLE Scan" to discover Unitree robots
5. Click discovered targets to select them
6. Use "START AUTOPWN" for automated exploitation
7. Monitor operations log for detailed status updates

### Manual Exploitation
- **Enable SSH**: Activates SSH daemon on target robot
- **Set Root Password**: Changes root password to known value
- **Custom Commands**: Execute arbitrary Linux commands via injection
- **System Information**: Extract robot configuration and status

## Development Architecture

### Core Components
- **BLE Stack**: ESP32 native BLE with custom Unitree protocol implementation
- **Web Server**: Async HTTP server with JSON API endpoints
- **Crypto Engine**: Hardware-accelerated AES encryption/decryption
- **UI Framework**: Responsive web interface with real-time updates

### Build System
- **PlatformIO**: Advanced embedded development platform
- **Custom Partitions**: Optimized flash layout for large web interface
- **Library Management**: Automated dependency resolution
- **Multi-board Support**: Configurable for different ESP32-S3 variants

## Legal and Ethical Notice

This tool implements published security research for educational and authorized testing purposes only. The original vulnerability research by Bin4ry and d0tslash was conducted responsibly and disclosed appropriately.

**Important**: Users must ensure they have explicit authorization before testing against any robotic systems. Unauthorized access to computer systems is illegal and unethical. This tool should only be used on systems you own or have written permission to test.

## Acknowledgments

- **Bin4ry**: Original UniPwn research and vulnerability discovery
- **d0tslash**: Co-researcher and exploitation development  
- **Security Research Community**: Responsible disclosure and knowledge sharing
- **Unitree Robotics**: For building innovative robotic platforms

## License

This educational implementation is provided for security research and authorized testing purposes. Users are responsible for compliance with all applicable laws and regulations in their jurisdiction.