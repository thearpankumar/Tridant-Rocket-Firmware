#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "board_config.h"

// Global objects
File logFile;

// Quick blink helper
void blinkLED(int times, int delayMs = 100) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
}

// State variables
bool sdReady = false;
uint32_t messagesReceived = 0;
uint32_t messagesValid = 0;
uint32_t messagesInvalid = 0;
uint32_t lastStatsTime = 0;

// Receive buffer
char rxBuffer[RX_BUFFER_SIZE];
uint16_t rxIndex = 0;

// Calculate XOR checksum
uint8_t calculateChecksum(const char* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= (uint8_t)data[i];
    }
    return checksum;
}

// Validate message checksum
// Format: $<DEVICE>,<MSG_ID>,<TYPE>,<SENSOR>,<VALUE>,<UNIT>,<TIMESTAMP>*<CHECKSUM>
bool validateChecksum(const char* message) {
    // Find the asterisk separator
    const char* asterisk = strchr(message, '*');
    if (!asterisk || message[0] != '$') {
        return false;
    }

    // Extract the received checksum (2 hex digits after *)
    char checksumStr[3] = {0};
    strncpy(checksumStr, asterisk + 1, 2);
    uint8_t receivedChecksum = (uint8_t)strtol(checksumStr, NULL, 16);

    // Calculate checksum of message body (between $ and *)
    size_t bodyLength = asterisk - message - 1;  // Exclude $
    uint8_t calculatedChecksum = calculateChecksum(message + 1, bodyLength);

    return receivedChecksum == calculatedChecksum;
}

// Send ACK response to ESP32
void sendAck(uint32_t msgId, bool valid) {
    char ackBuffer[TX_BUFFER_SIZE];
    const char* status = valid ? "OK" : "ERR";
    snprintf(ackBuffer, sizeof(ackBuffer), "ACK,%04lu,%s\n", msgId % 10000, status);
    UART_SERIAL.print(ackBuffer);
}

// Log message to SD card
void logToSD(const char* message, bool valid) {
    if (!sdReady) return;

    // Open file in append mode
    logFile = SD.open(LOG_FILENAME, FILE_WRITE);
    if (logFile) {
        // Add timestamp and validity marker
        logFile.print(millis());
        logFile.print(",");
        logFile.print(valid ? "VALID" : "INVALID");
        logFile.print(",");
        logFile.println(message);
        logFile.close();

        DEBUG_SERIAL.print("[SD] Logged: ");
        DEBUG_SERIAL.println(valid ? "VALID" : "INVALID");

        // Blink LED on successful write
        blinkLED(1, 50);
    } else {
        DEBUG_SERIAL.println("[SD] ERROR: Failed to open log file");
    }
}

// Extract message ID from message
uint32_t extractMessageId(const char* message) {
    // Format: $<DEVICE>,<MSG_ID>,...
    const char* comma1 = strchr(message, ',');
    if (!comma1) return 0;

    char idStr[5] = {0};
    strncpy(idStr, comma1 + 1, 4);
    return strtoul(idStr, NULL, 10);
}

// Process a complete received message
void processMessage(const char* message) {
    messagesReceived++;

    DEBUG_SERIAL.print("[RX] ");
    DEBUG_SERIAL.println(message);

    // Validate checksum
    bool valid = validateChecksum(message);

    if (valid) {
        messagesValid++;
        DEBUG_SERIAL.println("[OK] Checksum valid");
    } else {
        messagesInvalid++;
        DEBUG_SERIAL.println("[ERR] Checksum invalid");
    }

    // Log to SD card
    logToSD(message, valid);

    // Send ACK back to ESP32
    uint32_t msgId = extractMessageId(message);
    sendAck(msgId, valid);
}

// Print statistics
void printStats() {
    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("========== Statistics ==========");
    DEBUG_SERIAL.printf("Messages received: %lu\n", messagesReceived);
    DEBUG_SERIAL.printf("Valid messages:    %lu\n", messagesValid);
    DEBUG_SERIAL.printf("Invalid messages:  %lu\n", messagesInvalid);
    if (messagesReceived > 0) {
        float successRate = (float)messagesValid / messagesReceived * 100.0;
        DEBUG_SERIAL.printf("Success rate:      %.1f%%\n", successRate);
    }
    DEBUG_SERIAL.println("================================");
    DEBUG_SERIAL.println();
}

void setup() {
    // Initialize LED first for visual feedback
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);  // LED OFF initially

    // Initialize debug serial (USB)
    DEBUG_SERIAL.begin(115200);
    while (!DEBUG_SERIAL && millis() < 3000); // Wait up to 3 seconds

    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("================================");
    DEBUG_SERIAL.println("Teensy 4.1 UART Receiver");
    DEBUG_SERIAL.println("================================");
    DEBUG_SERIAL.printf("UART Baud Rate: %d\n", UART_BAUD);
    DEBUG_SERIAL.printf("Log File: %s\n", LOG_FILENAME);
    DEBUG_SERIAL.println("================================");

    // Initialize UART from ESP32
    UART_SERIAL.begin(UART_BAUD);

    // Initialize SD card
    DEBUG_SERIAL.print("Initializing SD card... ");
    if (SD.begin(SD_CS_PIN)) {
        sdReady = true;
        DEBUG_SERIAL.println("OK");

        // 3 quick blinks = SD card initialized successfully
        blinkLED(3);

        // Create/open log file and write header
        logFile = SD.open(LOG_FILENAME, FILE_WRITE);
        if (logFile) {
            logFile.println("=== UART Log Started ===");
            logFile.print("Boot time: ");
            logFile.println(millis());
            logFile.println("Format: timestamp,validity,message");
            logFile.println("========================");
            logFile.close();
            DEBUG_SERIAL.println("Log file ready");
        }
    } else {
        DEBUG_SERIAL.println("FAILED");
        DEBUG_SERIAL.println("WARNING: Continuing without SD logging");
    }

    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("Waiting for data from ESP32...");
    DEBUG_SERIAL.println();
}

void loop() {
    // Read UART data
    while (UART_SERIAL.available()) {
        char c = UART_SERIAL.read();

        if (c == '\n' || c == '\r') {
            if (rxIndex > 0) {
                rxBuffer[rxIndex] = '\0';
                processMessage(rxBuffer);
                rxIndex = 0;
            }
        } else if (rxIndex < RX_BUFFER_SIZE - 1) {
            rxBuffer[rxIndex++] = c;
        } else {
            // Buffer overflow - reset
            DEBUG_SERIAL.println("[ERR] Buffer overflow");
            rxIndex = 0;
        }
    }

    // Print statistics periodically
    if (millis() - lastStatsTime >= STATS_INTERVAL_MS) {
        lastStatsTime = millis();
        if (messagesReceived > 0) {
            printStats();
        }
    }
}
