#ifndef DUAL_LORA_COMM_H
#define DUAL_LORA_COMM_H

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "board_config.h"

// Number of LoRa modules
#define NUM_LORA_MODULES 2

// Module indices
#define MODULE_1 0
#define MODULE_2 1

// Define module type alias based on build flag
#if defined(LORA_MODULE_SX1262)
    typedef SX1262 LoRaModuleType;
#else
    typedef SX1278 LoRaModuleType;
#endif

class DualLoRaComm {
public:
    DualLoRaComm();
    ~DualLoRaComm();

    // Initialize both LoRa modules
    bool begin();

    // Send packet via specified module (0 or 1)
    bool sendPacket(uint8_t moduleIndex, const uint8_t* data, size_t length);

    // Get device name for module
    const char* getDeviceName(uint8_t moduleIndex);

    // Print configuration for debugging
    void printConfig();

private:
    // RadioLib Module instances
    Module* radioModules[NUM_LORA_MODULES];
    LoRaModuleType* radios[NUM_LORA_MODULES];

    // Module names
    const char* deviceNames[NUM_LORA_MODULES];

    // Pin configurations
    int nssPins[NUM_LORA_MODULES];
    int resetPins[NUM_LORA_MODULES];

    #if defined(LORA_MODULE_SX1262)
        int dio1Pins[NUM_LORA_MODULES];
        int busyPins[NUM_LORA_MODULES];
    #else
        int dio0Pins[NUM_LORA_MODULES];
    #endif

    // Initialization state
    bool initialized;

    // Initialize a single module
    bool initModule(uint8_t index);

    // Configure LoRa parameters for a module
    bool configureModule(LoRaModuleType* radio);

    // Select a module for communication (deselect others)
    void selectModule(uint8_t moduleIndex);
};

#endif // DUAL_LORA_COMM_H
