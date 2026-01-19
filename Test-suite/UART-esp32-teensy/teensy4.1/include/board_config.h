#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ==========================================
// Teensy 4.1 UART Receiver Configuration
// ==========================================

// UART1 Pin Configuration (connect to ESP32 Serial2)
// Pin 0 (RX1) <- ESP32 GPIO17 (TX2)
// Pin 1 (TX1) -> ESP32 GPIO16 (RX2)
#define UART_RX_PIN 0
#define UART_TX_PIN 1

#ifndef UART_BAUD
#define UART_BAUD 115200
#endif

// LED Configuration
#ifndef LED_PIN
#define LED_PIN 13  // Built-in LED on Teensy 4.1
#endif

// SD Card Configuration
// Teensy 4.1 has built-in SD card slot
#define SD_CS_PIN BUILTIN_SDCARD
#define LOG_FILENAME "uart_log.txt"

// Buffer Sizes
#define RX_BUFFER_SIZE 256
#define TX_BUFFER_SIZE 64

// Timing Configuration
#ifndef SD_FLUSH_INTERVAL_MS
#define SD_FLUSH_INTERVAL_MS 5000  // Flush SD every 5 seconds
#endif

#ifndef STATS_INTERVAL_MS
#define STATS_INTERVAL_MS 10000  // Print stats every 10 seconds
#endif

// Serial Aliases for cleaner code
#define DEBUG_SERIAL Serial   // USB Serial for debugging
#define UART_SERIAL  Serial1  // UART from ESP32

#endif // BOARD_CONFIG_H
