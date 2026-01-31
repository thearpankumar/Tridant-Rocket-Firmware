#ifndef TEENSY_UART_CONFIG_H
#define TEENSY_UART_CONFIG_H

// ============================================================================
// Teensy UART Configuration
// ============================================================================
// UART communication from ESP32 to Teensy for thrust data streaming
// Uses same protocol as UART-esp32-teensy project for compatibility

// ===== Pin Configuration (Serial2 remapped) =====
// GPIO16 is used by HX711 DOUT, so we remap Serial2 pins
#ifndef TEENSY_UART_TX_PIN
#define TEENSY_UART_TX_PIN 17      // ESP32 GPIO17 → Teensy RX1 (Pin 0)
#endif

#ifndef TEENSY_UART_RX_PIN
#define TEENSY_UART_RX_PIN 5       // ESP32 GPIO5  ← Teensy TX1 (Pin 1)
#endif

#ifndef TEENSY_UART_BAUD
#define TEENSY_UART_BAUD 115200    // Match Teensy receiver
#endif

// ===== Protocol Configuration =====
#define DEVICE_NAME "thrust_test"
#define SENSOR_NAME "THST"
#define SENSOR_UNIT "N"

// ===== Wiring Guide =====
// ESP32           Teensy 4.1
// =====           ==========
// GPIO17 (TX) --> Pin 0 (RX1)
// GPIO5  (RX) <-- Pin 1 (TX1)
// GND         <-> GND

#endif // TEENSY_UART_CONFIG_H
