#ifndef TEENSY_UART_H
#define TEENSY_UART_H

#include <Arduino.h>
#include "teensy_uart_config.h"

class TeensyUART {
public:
    TeensyUART();

    // Initialize Serial2 with remapped pins
    void begin();

    // Send thrust data to Teensy
    // Format: $thrust_test,XXXX,DATA,THST,XXX.XXX,N,XXXXX*XX\n
    void sendThrustData(float forceN, unsigned long timestampMs);

    // Non-blocking check for ACK from Teensy (optional)
    bool checkAck();

    // Get current message ID
    uint16_t getMessageId() const { return _messageId; }

private:
    // Calculate XOR checksum of message body
    uint8_t calculateChecksum(const char* msg, size_t length);

    uint16_t _messageId;
    char _txBuffer[128];
    char _rxBuffer[64];
    uint8_t _rxIndex;
};

#endif // TEENSY_UART_H
