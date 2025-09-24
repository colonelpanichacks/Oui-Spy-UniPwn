/*
 * Hardware Feedback System - Buzzer and LED Management
 * Based on OUI Spy Foxhunter functionality
 * Provides audio and visual feedback for UniPwn exploitation
 */

#if ENABLE_BUZZER || ENABLE_LED_FEEDBACK

// Buzzer state management (non-blocking)
bool beepActive = false;
unsigned long beepStartTime = 0;
unsigned long lastBeepTime = 0;
bool sessionFirstDetection = true;

// LED state management
#if ENABLE_LED_FEEDBACK
bool ledActive = false;
unsigned long ledStartTime = 0;
unsigned long lastLEDTime = 0;
#endif

// ================================
// Buzzer Functions (from OUI Spy Foxhunter)
// ================================
#if ENABLE_BUZZER

void initializeBuzzer() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    // Use new ESP32 Arduino Core 3.x LEDC API (works with current core version)
    ledcAttach(BUZZER_PIN, BUZZER_FREQ, 8);
}

void singleBeep() {
    if (!buzzerEnabled) return;
    ledcWrite(BUZZER_PIN, BUZZER_DUTY);
    delay(BEEP_DURATION);
    ledcWrite(BUZZER_PIN, 0);
}

void doubleBeep() {
    for(int i = 0; i < 2; i++) {
        singleBeep();
        if (i < 1) delay(BEEP_PAUSE);
    }
}

void tripleBeep() {
    for(int i = 0; i < 3; i++) {
        singleBeep();
        if (i < 2) delay(BEEP_PAUSE);
    }
}

void bootBeep() {
    if (!buzzerEnabled) return;
    // Single beep at boot
    singleBeep();
}

void scanningBeeps() {
    if (!buzzerEnabled) return;
    // 2 ascending beeps at scanning start
    ledcChangeFrequency(BUZZER_PIN, 1500, 8);  // Lower tone
    ledcWrite(BUZZER_PIN, BUZZER_DUTY);
    delay(BEEP_DURATION);
    ledcWrite(BUZZER_PIN, 0);
    delay(BEEP_PAUSE);
    
    ledcChangeFrequency(BUZZER_PIN, 2000, 8);  // Higher tone
    ledcWrite(BUZZER_PIN, BUZZER_DUTY);
    delay(BEEP_DURATION);
    ledcWrite(BUZZER_PIN, 0);
    
    // Reset to default frequency
    ledcChangeFrequency(BUZZER_PIN, BUZZER_FREQ, 8);
    
    // Pause 2 seconds before detections can occur
    delay(2000);
}

void heartbeatBeeps() {
    if (!buzzerEnabled) return;
    // 2 intermittent beeps like a heartbeat for device still around
    ledcWrite(BUZZER_PIN, BUZZER_DUTY);
    delay(100);  // Short beep
    ledcWrite(BUZZER_PIN, 0);
    delay(100);  // Short pause
    ledcWrite(BUZZER_PIN, BUZZER_DUTY);
    delay(150);  // Slightly longer beep
    ledcWrite(BUZZER_PIN, 0);
}

void ascendingBeeps() {
    if (!buzzerEnabled) return;
    // Ready signal - 2 fast ascending beeps with close melodic notes
    ledcChangeFrequency(BUZZER_PIN, 1900, 8);
    ledcWrite(BUZZER_PIN, BUZZER_DUTY);
    delay(150);
    ledcWrite(BUZZER_PIN, 0);
    delay(50);
    
    ledcChangeFrequency(BUZZER_PIN, 2200, 8);
    ledcWrite(BUZZER_PIN, BUZZER_DUTY);
    delay(150);
    ledcWrite(BUZZER_PIN, 0);
    
    // Reset to normal frequency and ENSURE buzzer is OFF
    ledcChangeFrequency(BUZZER_PIN, BUZZER_FREQ, 8);
    ledcWrite(BUZZER_PIN, 0);  // Make sure buzzer is completely off
}

void botDetectionBeeps() {
    if (!buzzerEnabled) return;
    // 3 fast beeps for bot detection (same pattern as LED)
    for (int i = 0; i < 3; i++) {
        ledcChangeFrequency(BUZZER_PIN, 2000, 8);
        ledcWrite(BUZZER_PIN, BUZZER_DUTY);
        delay(150);
        ledcWrite(BUZZER_PIN, 0);
        if (i < 2) delay(100);
    }
    
    // Reset to normal frequency
    ledcChangeFrequency(BUZZER_PIN, BUZZER_FREQ, 8);
    ledcWrite(BUZZER_PIN, 0);
}

void exploitSuccessBeeps() {
    if (!buzzerEnabled) return;
    // Success pattern: ascending triplet (indicates successful exploitation)
    int frequencies[] = {1800, 2100, 2400};
    
    for (int i = 0; i < 3; i++) {
        ledcChangeFrequency(BUZZER_PIN, frequencies[i], 8);
        ledcWrite(BUZZER_PIN, BUZZER_DUTY);
        delay(200);
        ledcWrite(BUZZER_PIN, 0);
        if (i < 2) delay(100);
    }
    
    // Reset to normal frequency
    ledcChangeFrequency(BUZZER_PIN, BUZZER_FREQ, 8);
    ledcWrite(BUZZER_PIN, 0);
}

void exploitFailedBeeps() {
    if (!buzzerEnabled) return;
    // Failure pattern: descending tone (indicates failed exploitation)
    ledcChangeFrequency(BUZZER_PIN, 2000, 8);
    ledcWrite(BUZZER_PIN, BUZZER_DUTY);
    delay(300);
    ledcChangeFrequency(BUZZER_PIN, 1600, 8);
    delay(300);
    ledcWrite(BUZZER_PIN, 0);
    
    // Reset to normal frequency
    ledcChangeFrequency(BUZZER_PIN, BUZZER_FREQ, 8);
    ledcWrite(BUZZER_PIN, 0);
}

void targetFoundBeeps() {
    if (!buzzerEnabled) return;
    // Target discovery: three same-tone beeps
    for (int i = 0; i < 3; i++) {
        ledcChangeFrequency(BUZZER_PIN, 2000, 8);
        ledcWrite(BUZZER_PIN, BUZZER_DUTY);
        delay(150);
        ledcWrite(BUZZER_PIN, 0);
        if (i < 2) delay(100);
    }
    
    // Reset to normal frequency
    ledcChangeFrequency(BUZZER_PIN, BUZZER_FREQ, 8);
    ledcWrite(BUZZER_PIN, 0);
}

void startupBeep() {
    // Simple boot beep - same volume as foxhunter
    ledcWrite(BUZZER_PIN, BUZZER_DUTY);  // Use proper duty cycle (127) like foxhunter
    delay(100);                          // Same duration as foxhunter singleBeep
    ledcWrite(BUZZER_PIN, 0);
}

// RSSI-based beep interval calculation (from foxhunter)
int calculateBeepInterval(int rssi) {
    // RSSI ranges: -95 (very weak) to -30 (very strong)
    if (rssi >= -35) {
        return map(rssi, -35, -25, 25, 10); // 25ms to 10ms - INSANE SPEED
    } else if (rssi >= -45) {
        return map(rssi, -45, -35, 75, 25); // 75ms to 25ms - MACHINE GUN
    } else if (rssi >= -55) {
        return map(rssi, -55, -45, 150, 75); // 150ms to 75ms - ULTRA FAST
    } else if (rssi >= -65) {
        return map(rssi, -65, -55, 250, 150); // 250ms to 150ms - VERY FAST
    } else if (rssi >= -75) {
        return map(rssi, -75, -65, 400, 250); // 400ms to 250ms - FAST
    } else if (rssi >= -85) {
        return map(rssi, -85, -75, 600, 400); // 600ms to 400ms - MEDIUM
    } else {
        return 800; // 800ms max for very weak signals (-85 and below)
    }
}

void startProximityBeep() {
    if (buzzerEnabled && !beepActive) {
        ledcChangeFrequency(BUZZER_PIN, 1000, 8);       // 1kHz tone
        ledcWrite(BUZZER_PIN, BUZZER_DUTY);    // Turn on buzzer
        beepActive = true;
        beepStartTime = millis();
    }
}

void stopProximityBeep() {
    if (beepActive) {
        ledcWrite(BUZZER_PIN, 0);              // Turn off buzzer
        beepActive = false;
    }
}

void handleProximityBeeping(int rssi) {
    if (!buzzerEnabled) return;
    
    unsigned long currentTime = millis();
    
    // Handle beep duration (100ms on)
    if (beepActive && (currentTime - beepStartTime >= 100)) {
        stopProximityBeep();
    }
    
    // Handle beep intervals based on RSSI
    int beepInterval = calculateBeepInterval(rssi);
    if (!beepActive && (currentTime - lastBeepTime >= beepInterval)) {
        startProximityBeep();
        lastBeepTime = currentTime;
    }
}

#endif // ENABLE_BUZZER

// ================================
// LED Functions 
// ================================
#if ENABLE_LED_FEEDBACK

void initializeLED() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // HIGH = OFF for XIAO ESP32-S3 (inverted logic)
}

void ledOn() {
    if (ledEnabled) {
        digitalWrite(LED_PIN, LOW);   // XIAO ESP32-S3 orange LED: LOW = ON (inverted logic)
    }
}

void ledOff() {
    if (ledEnabled) {
        digitalWrite(LED_PIN, HIGH);  // XIAO ESP32-S3 orange LED: HIGH = OFF (inverted logic)
    }
}

void ledBlink(int duration = 100) {
    ledOn();
    delay(duration);
    ledOff();
}

void ledBlinkPattern(int count, int onTime = 100, int offTime = 100) {
    for (int i = 0; i < count; i++) {
        ledOn();
        delay(onTime);
        ledOff();
        if (i < count - 1) delay(offTime);
    }
}

void ledExploitSuccess() {
    // Success pattern: 3 quick blinks
    ledBlinkPattern(3, 150, 100);
}

void ledExploitFailed() {
    // Failed pattern: 2 long blinks
    ledBlinkPattern(2, 500, 300);
}

void ledTargetFound() {
    // Target found: Single long blink
    ledBlink(300);
}

void ledScanning() {
    // Scanning pattern: Quick double blink
    ledBlinkPattern(2, 50, 50);
}

void ledConnecting() {
    // Connecting pattern: Rapid blinks
    ledBlinkPattern(5, 30, 30);
}

void startProximityLED() {
    if (ledEnabled && !ledActive) {
        ledOn();
        ledActive = true;
        ledStartTime = millis();
    }
}

void stopProximityLED() {
    if (ledActive) {
        ledOff();
        ledActive = false;
    }
}

void handleProximityLED(int rssi) {
    if (!ledEnabled) return;
    
    unsigned long currentTime = millis();
    int ledInterval = calculateBeepInterval(rssi) / 2; // LED blinks twice as fast as beeps
    
    // Handle LED duration (50ms on)
    if (ledActive && (currentTime - ledStartTime >= 50)) {
        stopProximityLED();
    }
    
    // Handle LED intervals based on RSSI
    if (!ledActive && (currentTime - lastLEDTime >= ledInterval)) {
        startProximityLED();
        lastLEDTime = currentTime;
    }
}

#endif // ENABLE_LED_FEEDBACK

// ================================
// Combined Feedback Functions
// ================================
void initializeHardwareFeedback() {
#if ENABLE_BUZZER
    initializeBuzzer();
    delay(100);  // Give buzzer time to initialize
#endif

#if ENABLE_LED_FEEDBACK
    initializeLED();
#endif

#if ENABLE_BUZZER
    // Boot beep
    startupBeep();
#endif
}

void feedbackExploitSuccess() {
#if ENABLE_BUZZER
    exploitSuccessBeeps();
#endif
#if ENABLE_LED_FEEDBACK
    ledExploitSuccess();
#endif
}

void feedbackExploitFailed() {
#if ENABLE_BUZZER
    exploitFailedBeeps();
#endif
#if ENABLE_LED_FEEDBACK
    ledExploitFailed();
#endif
}

void feedbackTargetFound() {
#if ENABLE_BUZZER
    targetFoundBeeps();
#endif
#if ENABLE_LED_FEEDBACK
    ledTargetFound();
#endif
}

void feedbackBotDetection() {
    // Combined bot detection feedback - 3 fast beeps and LED pattern
#if ENABLE_BUZZER
    botDetectionBeeps();
#endif
#if ENABLE_LED_FEEDBACK
    ledBlinkPattern(3, 150, 100); // Same pattern as beeps
#endif
}

void feedbackScanning() {
#if ENABLE_LED_FEEDBACK
    ledScanning();
#endif
}

void feedbackConnecting() {
#if ENABLE_BUZZER
    doubleBeep();
#endif
#if ENABLE_LED_FEEDBACK
    ledConnecting();
#endif
}

void handleProximityFeedback(int rssi) {
#if ENABLE_BUZZER
    handleProximityBeeping(rssi);
#endif
#if ENABLE_LED_FEEDBACK
    handleProximityLED(rssi);
#endif
}

void stopAllFeedback() {
#if ENABLE_BUZZER
    stopProximityBeep();
    ledcWrite(BUZZER_PIN, 0);
#endif
#if ENABLE_LED_FEEDBACK
    stopProximityLED();
#endif
}

// Menu toggle functions
void toggleBuzzer() {
#if ENABLE_BUZZER
    buzzerEnabled = !buzzerEnabled;
    styledPrint("Buzzer " + String(buzzerEnabled ? "ENABLED" : "DISABLED"));
    
    // Stop any active buzzing when disabled
    if (!buzzerEnabled) {
        stopProximityBeep();
        ledcWrite(BUZZER_PIN, 0);
    } else {
        // Test beep when enabled
        singleBeep();
    }
    
    // Save the setting
    saveConfiguration();
#else
    styledPrint("Buzzer support not compiled in");
#endif
}

void toggleLED() {
#if ENABLE_LED_FEEDBACK
    ledEnabled = !ledEnabled;
    styledPrint("LED feedback " + String(ledEnabled ? "ENABLED" : "DISABLED"));
    
    // Turn off LED when disabled
    if (!ledEnabled) {
        stopProximityLED();
        digitalWrite(LED_PIN, HIGH);  // HIGH = OFF (inverted logic)
    } else {
        // Test blink when enabled
        ledBlink(200);
    }
    
    // Save the setting
    saveConfiguration();
#else
    styledPrint("LED feedback support not compiled in");
#endif
}

#endif // ENABLE_BUZZER || ENABLE_LED_FEEDBACK
