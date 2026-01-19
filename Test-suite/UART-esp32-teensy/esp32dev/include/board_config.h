#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ==========================================
// ESP32 DevKit UART Configuration
// ==========================================

// UART2 Pin Configuration (connect to Teensy Serial1)
#ifndef UART_TX_PIN
#define UART_TX_PIN 17  // GPIO17 -> Teensy Pin 0 (RX1)
#endif

#ifndef UART_RX_PIN
#define UART_RX_PIN 16  // GPIO16 <- Teensy Pin 1 (TX1)
#endif

#ifndef UART_BAUD
#define UART_BAUD 115200
#endif

// LED Configuration
#ifndef LED_PIN
#define LED_PIN 2  // Built-in LED on most ESP32 DevKit boards
#endif

// Timing Configuration
#ifndef SEND_INTERVAL_MS
#define SEND_INTERVAL_MS 1000  // Send data every 1 second
#endif

// Device Configuration
#define DEVICE_NAME "esp32_sender"

// Serial Aliases for cleaner code
#define DEBUG_SERIAL Serial      // USB Serial for debugging
#define UART_SERIAL  Serial2     // UART to Teensy

// Pin validation macros
#define IS_VALID_GPIO(pin) ((pin) >= 0 && (pin) <= 39)
#define VALIDATE_PINS() \
    static_assert(IS_VALID_GPIO(UART_TX_PIN), "Invalid UART TX pin"); \
    static_assert(IS_VALID_GPIO(UART_RX_PIN), "Invalid UART RX pin")

#endif // BOARD_CONFIG_H
