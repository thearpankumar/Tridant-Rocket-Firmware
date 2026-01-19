#include <Arduino.h>
#include "board_config.h"
#include "DummySensors.h"

// Validate pin configuration at compile time
VALIDATE_PINS();

// Global objects
DummySensors sensors;

// State variables
uint32_t messageId = 0;
uint32_t lastSendTime = 0;
uint8_t currentSensorIndex = 0;

// Buffer for UART messages
char txBuffer[128];
char rxBuffer[64];
uint8_t rxIndex = 0;

// Calculate XOR checksum and return as hex string
uint8_t calculateChecksum(const char* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= (uint8_t)data[i];
    }
    return checksum;
}

// Format and send sensor data
void sendSensorData(uint8_t sensorId) {
    float value = sensors.readSensorById(sensorId);
    const char* sensorName = sensors.getSensorNameShort(sensorId);
    const char* unit = sensors.getSensorUnit(sensorId);
    uint32_t timestamp = millis();

    // Format: $<DEVICE>,<MSG_ID>,<TYPE>,<SENSOR>,<VALUE>,<UNIT>,<TIMESTAMP>*<CHECKSUM>\n
    // Build message without checksum first
    char msgBody[100];
    snprintf(msgBody, sizeof(msgBody), "$%s,%04lu,DATA,%s,%.2f,%s,%lu",
             DEVICE_NAME, messageId % 10000, sensorName, value, unit, timestamp);

    // Calculate checksum of message body (excluding $)
    uint8_t checksum = calculateChecksum(msgBody + 1, strlen(msgBody) - 1);

    // Format complete message with checksum
    snprintf(txBuffer, sizeof(txBuffer), "%s*%02X\n", msgBody, checksum);

    // Send via UART
    UART_SERIAL.print(txBuffer);

    // Debug output
    DEBUG_SERIAL.print("[TX] ");
    DEBUG_SERIAL.print(txBuffer);

    // Blink LED
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);

    messageId++;
}

// Process received ACK messages
void processReceivedData() {
    while (UART_SERIAL.available()) {
        char c = UART_SERIAL.read();

        if (c == '\n' || c == '\r') {
            if (rxIndex > 0) {
                rxBuffer[rxIndex] = '\0';
                DEBUG_SERIAL.print("[RX] ");
                DEBUG_SERIAL.println(rxBuffer);
                rxIndex = 0;
            }
        } else if (rxIndex < sizeof(rxBuffer) - 1) {
            rxBuffer[rxIndex++] = c;
        }
    }
}

void setup() {
    // Initialize debug serial (USB)
    DEBUG_SERIAL.begin(115200);
    while (!DEBUG_SERIAL && millis() < 3000); // Wait up to 3 seconds

    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("================================");
    DEBUG_SERIAL.println("ESP32 UART Sender");
    DEBUG_SERIAL.println("================================");
    DEBUG_SERIAL.printf("TX Pin: GPIO%d\n", UART_TX_PIN);
    DEBUG_SERIAL.printf("RX Pin: GPIO%d\n", UART_RX_PIN);
    DEBUG_SERIAL.printf("Baud Rate: %d\n", UART_BAUD);
    DEBUG_SERIAL.printf("Send Interval: %d ms\n", SEND_INTERVAL_MS);
    DEBUG_SERIAL.println("================================");

    // Initialize UART to Teensy (Serial2 with custom pins)
    UART_SERIAL.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialize sensors
    sensors.begin();

    DEBUG_SERIAL.println("Initialization complete. Starting transmission...");
    DEBUG_SERIAL.println();
}

void loop() {
    // Check for received data (ACKs from Teensy)
    processReceivedData();

    // Send sensor data at interval
    if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
        lastSendTime = millis();

        // Get current sensor ID (1-based: TEMP=1, HUMID=2, BAT=3, PRES=4)
        uint8_t sensorId = (currentSensorIndex % SENSOR_COUNT) + 1;
        sendSensorData(sensorId);

        currentSensorIndex++;
    }
}
