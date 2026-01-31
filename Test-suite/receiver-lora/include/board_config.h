#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Pin definitions are provided via build flags in platformio.ini
// This header validates that all required pins are defined at compile time

// ===== Module Type Validation =====
#if !defined(LORA_MODULE_SX1262) && !defined(LORA_MODULE_RA02)
    #error "LORA_MODULE type not defined. Use -D LORA_MODULE_RA02 or -D LORA_MODULE_SX1262"
#endif

#if defined(LORA_MODULE_SX1262) && defined(LORA_MODULE_RA02)
    #error "Only one LORA_MODULE type can be defined"
#endif

// ===== Common Pin Validation =====
#ifndef LORA_NSS
    #error "LORA_NSS not defined. Check platformio.ini build_flags"
#endif

#ifndef LORA_RESET
    #error "LORA_RESET not defined. Check platformio.ini build_flags"
#endif

#ifndef LORA_FREQUENCY
    #error "LORA_FREQUENCY not defined. Check platformio.ini build_flags"
#endif

#ifndef BOARD_NAME
    #error "BOARD_NAME not defined. Check platformio.ini build_flags"
#endif

// ===== Module-Specific Pin Validation =====
#if defined(LORA_MODULE_SX1262)
    // SX1262 requires DIO1 and BUSY pins
    #ifndef LORA_DIO1
        #error "LORA_DIO1 required for SX1262. Check platformio.ini build_flags"
    #endif
    #ifndef LORA_BUSY
        #error "LORA_BUSY required for SX1262. Check platformio.ini build_flags"
    #endif
#else
    // Ra-02 (SX1278) requires DIO0 pin
    #ifndef LORA_DIO0
        #error "LORA_DIO0 required for Ra-02/SX1278. Check platformio.ini build_flags"
    #endif
#endif

#ifndef LED_PIN
    #define LED_PIN LED_BUILTIN  // Default to built-in LED if not defined
#endif

// LoRa Configuration Parameters
#define LORA_SPREADING_FACTOR 7         // SF7-SF12 (7=fast/short, 12=slow/long)
#define LORA_SIGNAL_BANDWIDTH 125E3     // 125 kHz bandwidth
#define LORA_CODING_RATE 5              // 4/5 coding rate
#define LORA_PREAMBLE_LENGTH 8          // Preamble length
#define LORA_SYNC_WORD 0x12             // Private network sync word
#define LORA_TX_POWER 17                // TX power in dBm (2-20)

// Serial Configuration
#define SERIAL_BAUD 9600

#endif // BOARD_CONFIG_H
