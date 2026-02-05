#include "TeensyUART.h"

TeensyUART::TeensyUART()
    : _messageId(0)
    , _rxIndex(0)
{
}

void TeensyUART::begin() {
    // Initialize Serial2 with custom pins (TX=GPIO17, RX=GPIO5)
    // This avoids GPIO16 which is used by HX711 DOUT
    Serial2.begin(TEENSY_UART_BAUD, SERIAL_8N1, TEENSY_UART_RX_PIN, TEENSY_UART_TX_PIN);

    Serial.println(F("# Teensy UART initialized"));
    Serial.print(F("#   TX Pin: GPIO"));
    Serial.println(TEENSY_UART_TX_PIN);
    Serial.print(F("#   RX Pin: GPIO"));
    Serial.println(TEENSY_UART_RX_PIN);
    Serial.print(F("#   Baud: "));
    Serial.println(TEENSY_UART_BAUD);
}

void TeensyUART::sendThrustData(float forceN, unsigned long timestampMs) {
    // Build message body (excluding checksum)
    // Format: $<DEVICE>,<MSG_ID>,<TYPE>,<SENSOR>,<VALUE>,<UNIT>,<TIMESTAMP>
    char msgBody[100];
    snprintf(msgBody, sizeof(msgBody), "$%s,%04lu,DATA,%s,%.3f,%s,%lu",
             DEVICE_NAME,
             (unsigned long)(_messageId % 10000),
             SENSOR_NAME,
             (double)forceN,
             SENSOR_UNIT,
             timestampMs);

    // Calculate checksum of message body (excluding leading $)
    uint8_t checksum = calculateChecksum(msgBody + 1, strlen(msgBody) - 1);

    // Format complete message with checksum
    snprintf(_txBuffer, sizeof(_txBuffer), "%s*%02X\n", msgBody, checksum);

    // Send via UART
    Serial2.print(_txBuffer);

    // Increment message ID (wraps at 10000)
    _messageId++;
    if (_messageId >= 10000) {
        _messageId = 0;
    }
}

bool TeensyUART::checkAck() {
    bool gotAck = false;

    while (Serial2.available()) {
        char c = Serial2.read();

        if (c == '\n' || c == '\r') {
            if (_rxIndex > 0) {
                _rxBuffer[_rxIndex] = '\0';
                // Check if it's an ACK message
                if (strncmp(_rxBuffer, "ACK,", 4) == 0) {
                    gotAck = true;
                }
                _rxIndex = 0;
            }
        } else if (_rxIndex < sizeof(_rxBuffer) - 1) {
            _rxBuffer[_rxIndex++] = c;
        }
    }

    return gotAck;
}

uint8_t TeensyUART::calculateChecksum(const char* msg, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= (uint8_t)msg[i];
    }
    return checksum;
}
